#include<RcppArmadillo.h>

#include<stdio.h>
#include<vector>

#include"BVCUtilities.h"
// [[Rcpp::depends(RcppArmadillo)]]
using namespace Rcpp;
using namespace arma;
using namespace std;
//using namespace R;

// [[Rcpp::export()]]

Rcpp::List BGL(arma::mat y, arma:: mat e, arma:: mat C, arma::mat g, arma:: mat w, arma::mat z,int maxSteps, int n, int k,arma::vec hatBeta, arma:: vec hatEta, arma::vec hatAlpha, arma::vec hatAta,arma::vec hatInvTauSq1, arma::vec hatInvTauSq2, arma::vec invSigAlpha0, double hatLambdaSqStar1, double hatLambdaSqStar2, double hatSigmaSq, double hatPhiSq,double aStar, double bStar, double alpha, double gamma,double alpha1,double gamma1)
{
  unsigned int q = e.n_cols,m = g.n_cols,p = w.n_cols,c = z.n_cols,o = C.n_cols;
  arma::mat gsAlpha(maxSteps, q+o),
  gsBeta(maxSteps,m),
  gseta(maxSteps,p),
  gsAta(maxSteps,n*c),
  gsInvTauSq1(maxSteps,m),
  gsInvTauSq2(maxSteps, m);
  
  
  arma::vec
  gsLambdaStar1(maxSteps),
  gsLambdaStar2(maxSteps),
  gsPhiSq(maxSteps),
  gsSigmaSq(maxSteps);
  
  
  
  
  arma::mat mat0,mat1,mat2,mat3,ei,gi,wi;
  
  
  double meanAlpha;
  double varAlpha;
  
  double meanb;
  double varb;
  
  
  for (int t = 0; t < maxSteps; t++) {
    
    
    mat0 = arma::repelem(e,k,1);
    mat1 = arma::repelem(g,k,1);
    mat2 = arma::repelem(w,k,1);
    
    // alpha|
    for(unsigned int j=0;j<q+o;j++){
      arma::vec res1, res11;
      double A0;
      A0 =0;
      double B0;
      B0=0;
      for(int i=0;i<n;i++){
        ei = mat0.rows((i*k),(i*k+k-1));
        ei.insert_cols(q, C);
        gi = mat1.rows((i*k),(i*k+k-1));
        wi = mat2.rows((i*k),(i*k+k-1));
        A0 = A0+arma::as_scalar(ei.col(j).t()*ei.col(j))/hatSigmaSq;
        res1 = y.row(i).t()-ei*hatAlpha-gi*hatBeta-wi*hatEta-z*hatAta.subvec((i*c),(i*c+c-1));
        res11 = res1+ei.col(j)*hatAlpha(j);
        B0 = B0+arma::as_scalar(ei.col(j).t()*res11)/hatSigmaSq;
        
      }
      
      
      meanAlpha = arma::as_scalar(B0/(invSigAlpha0(j)+A0));
      
      
      varAlpha = arma::as_scalar(1/(invSigAlpha0(j)+A0));
      
      hatAlpha(j) = R::rnorm(meanAlpha,sqrt(varAlpha));
      
    }
    
    
    gsAlpha.row(t) = hatAlpha.t();
    
    // ata|
    arma::vec res;
    for(int i=0;i<n;i++){
      ei = mat0.rows((i*k),(i*k+k-1));
      ei.insert_cols(q, C);
      gi = mat1.rows((i*k),(i*k+k-1));
      wi = mat2.rows((i*k),(i*k+k-1));
      arma::mat tzz =  z.t()*z;
      arma::mat A = tzz/hatSigmaSq;
      res = y.row(i).t()-ei*hatAlpha-gi*hatBeta-wi*hatEta;
      arma::vec B = z.t()*res/hatSigmaSq;
      arma:: mat T(c,c);
      T = T.eye();
      arma::mat invhatPhiSq = 1/hatPhiSq*T;
      arma::mat varAta = arma::inv(A+invhatPhiSq);
      arma::vec meanAta = varAta*B;
      hatAta.subvec((i*c),(i*c+c-1)) = mvrnormCpp(meanAta, varAta);
    }
    gsAta.row(t) = hatAta.t();
    
    
    // Beta|
    for(unsigned int j=0;j<m;j++){
      arma::vec res2, res22;
      double A1;
      A1=0;
      double B1;
      B1=0;
      
      for(int i=0;i<n;i++){
        ei = mat0.rows((i*k),(i*k+k-1));
        ei.insert_cols(q, C);
        gi = mat1.rows((i*k),(i*k+k-1));
        wi = mat2.rows((i*k),(i*k+k-1));
        arma::vec tggDiag = sum(square(gi),0).t();
        A1 = A1+tggDiag(j);
        res2 = y.row(i).t()-ei*hatAlpha-gi*hatBeta-wi*hatEta-z*hatAta.subvec((i*c),(i*c+c-1));
        res22 = res2+gi.col(j)*hatBeta(j);
        B1 = B1+ arma::as_scalar(gi.col(j).t()*res22);
        
      }
      meanb = arma::as_scalar(B1/(hatInvTauSq1(j)+A1));
      
      varb = arma::as_scalar(hatSigmaSq/(hatInvTauSq1(j)+A1));
      
      
      hatBeta(j)=R::rnorm(meanb,sqrt(varb));
      
    }
    
    gsBeta.row(t) = hatBeta.t();
    
    
    // eta|
    
    arma::vec meane;
    arma::mat varcove(q,q);
    
    arma:: mat Tau(q,q);
    Tau = Tau.eye();
    
    
    for(unsigned int j=0;j<m;j++){
      arma::mat A2(q,q);
      A2 = A2.zeros();
      arma:: vec B2;
      B2 = zeros<vec>(q);
      
      arma::vec res3, res33;
      
      for(int i=0;i<n;i++){
        ei = mat0.rows((i*k),(i*k+k-1));
        ei.insert_cols(q, C);
        gi = mat1.rows((i*k),(i*k+k-1));
        wi = mat2.rows((i*k),(i*k+k-1));
        A2 = A2+wi.cols((j*q),(j*q+q-1)).t()*wi.cols((j*q),(j*q+q-1));
        res3 = y.row(i).t()-ei*hatAlpha-gi*hatBeta-wi*hatEta-z*hatAta.subvec((i*c),(i*c+c-1));
        res33 = res3+wi.cols((j*q),(j*q+q-1))*hatEta.subvec((j*q),(j*q+q-1));
        B2 = B2+wi.cols((j*q),(j*q+q-1)).t()*res33;
        
      }
      
      meane = arma::inv(hatInvTauSq2(j)*Tau+A2)*B2;
      
      varcove = hatSigmaSq*arma::inv(hatInvTauSq2(j)*Tau+A2);
      
      
      hatEta.subvec((j*q),(j*q+q-1)) = mvrnormCpp(meane,varcove);
      
      
    }
    
    gseta.row(t) = hatEta.t();
    
    
    // invTAUsq.star1|
    
    double lInvTauSq1; 
    lInvTauSq1   = hatLambdaSqStar1;
    arma::vec muInvTauSq1; 
    muInvTauSq1 = arma::sqrt(hatLambdaSqStar1 * hatSigmaSq / arma::square(hatBeta));
    for(unsigned int j = 0; j<m; j++){
      hatInvTauSq1(j) = rinvgaussian(muInvTauSq1(j), lInvTauSq1);
    }
    
    
    gsInvTauSq1.row(t) = hatInvTauSq1.t();
    
    // invTAUsq.star2|
    double lInvTauSq2; 
    arma:: mat matRStar;
    arma:: vec tRsRs, muInvTauSq2;
    lInvTauSq2 = q*hatLambdaSqStar2;
    matRStar = arma::reshape(hatEta, q, m);
    tRsRs = sum(arma::square(matRStar), 0).t();
    muInvTauSq2 = arma::sqrt(q * hatLambdaSqStar2 * hatSigmaSq / tRsRs);
    for(unsigned int j = 0; j<m; j++){
      hatInvTauSq2(j) = rinvgaussian(muInvTauSq2(j), lInvTauSq2);
    }
    
    gsInvTauSq2.row(t) = hatInvTauSq2.t();
    
    
    // sigma.sq|
    arma:: vec repInvTauSq;
    double shapeSig, rateSig;
    shapeSig = alpha + n*k/2 + m/2+ p/2;
    double ress;
    ress=0;
    for(int i=0;i<n;i++){
      ei = mat0.rows((i*k),(i*k+k-1));
      ei.insert_cols(q, C);
      gi = mat1.rows((i*k),(i*k+k-1));
      wi = mat2.rows((i*k),(i*k+k-1));
      arma::vec res4;
      res4 = y.row(i).t()-ei*hatAlpha-gi*hatBeta-wi*hatEta-z*hatAta.subvec((i*c),(i*c+c-1));
      ress = ress+arma::accu(arma::square(res4));
    }
    repInvTauSq = arma::vectorise(arma::repelem(hatInvTauSq2.t(), q, 1), 0);
    rateSig = gamma + 0.5*(ress +arma::accu(square(hatBeta)%hatInvTauSq1)+ arma::accu(square(hatEta) % repInvTauSq));
    
    hatSigmaSq = 1/R::rgamma(shapeSig, 1/rateSig);
    gsSigmaSq(t) = hatSigmaSq;
    
    // phi.sq|
    double shapePhi, ratePhi;
    shapePhi = alpha1 + n*c/2;
    double diff;
    diff=0;
    for(int i=0;i<n;i++){
      diff= diff+0.5*(arma::accu(square(hatAta.subvec((i*c),(i*c+c-1)))));
    }
    
    ratePhi = gamma1 + diff;
    hatPhiSq = 1/R::rgamma(shapePhi, 1/ratePhi);
    gsPhiSq(t) = hatPhiSq;
    
    // lambda.star1|
    double shapeS1;
    shapeS1= aStar + m;
    double rateS1;
    rateS1= bStar + arma::accu(1/hatInvTauSq1)/2;
    hatLambdaSqStar1 = R::rgamma(shapeS1, 1/rateS1);
    gsLambdaStar1(t) = hatLambdaSqStar1;
    
    // lambda.star2|
    double shapeS2;
    shapeS2= aStar + m*(q+1)/2;
    double rateS2;
    rateS2= bStar + q*arma::accu(1/hatInvTauSq2)/2;
    hatLambdaSqStar2 = R::rgamma(shapeS2, 1/rateS2);
    gsLambdaStar2(t) = hatLambdaSqStar2;
    
    
  }
  
  return Rcpp::List::create(
    Rcpp::Named("GS.alpha") = gsAlpha,
    Rcpp::Named("GS.beta") = gsBeta,
    Rcpp::Named("GS.eta") = gseta,
    Rcpp::Named("GS.ata") = gsAta,
    Rcpp::Named("GS.invTAUsq1") = gsInvTauSq1,
    Rcpp::Named("GS.invTAUsq2") = gsInvTauSq2,
    Rcpp::Named("GS.lambda.sq1") = gsLambdaStar1,
    Rcpp::Named("GS.lambda.sq2") = gsLambdaStar2,
    Rcpp::Named("GS.phi.sq") = gsPhiSq,
    Rcpp::Named("GS.sigma.sq") = gsSigmaSq
  
  );
  
}

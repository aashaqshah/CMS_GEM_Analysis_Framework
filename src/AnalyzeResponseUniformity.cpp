//
//  AnalyzeResponseUniformity.cpp
//  
//
//  Created by Brian L Dorney on 28/01/16.
//
//

#include "DetectorMPGD.h"
#include "AnalyzeResponseUniformity.h"

using std::cout;
using std::endl;
using std::make_shared;
using std::map;
using std::shared_ptr;
using std::string;
using std::vector;

using Timing::getString;
using Timing::printROOTFileStatus;
using Timing::HistoSetup;
using Timing::stofSafe;

using namespace Uniformity;

//Default Constructor
AnalyzeResponseUniformity::AnalyzeResponseUniformity(){
    strAnalysisName = "analysis";
} //End Default Constructor

//Set inputs at construction
AnalyzeResponseUniformity::AnalyzeResponseUniformity(AnalysisSetupUniformity inputSetup, DetectorMPGD & inputDet){
    strAnalysisName = "analysis";
    
    //Store Analysis Parameters
    aSetup = inputSetup;
    
    //Store Detector
    detMPGD = inputDet;
} //End Constructor

//Checks to see if the detector's uniformity is within requested amount
/*void AnalyzeResponseUniformity::checkUniformity(){
    //Check to make sure at least one peak exists
    if(detMPGD.vec_allADCPeaks.size() == 0){
	cout<<"============Analysis Summary============\n";
    	cout<<"No Stored ADC Peaks; All Fits Failed!?\n";
	cout<<"You should Investigate the output data file\n";
	return;
    }

    //Variable Declaration
    float fResponse_Max = *(std::max_element(detMPGD.vec_allADCPeaks.begin(), detMPGD.vec_allADCPeaks.end() ) );
    float fResponse_Min = *(std::min_element(detMPGD.vec_allADCPeaks.begin(), detMPGD.vec_allADCPeaks.end() ) );
    float fResponse_Var = (fResponse_Max - fResponse_Min) / fResponse_Max;
    
    //Output Above Info to User
    cout<<"============Analysis Summary============\n";
    cout<<"Detector's Minimum Response: " << fResponse_Min << endl;
    cout<<"Detector's Maximum Response: " << fResponse_Max << endl;
    cout<<"Calculated Difference: " << fResponse_Var << endl;

    //Check if Detector is within tolerance
    if ( fabs(fResponse_Var) <= aSetup.fUniformityTolerance ) { //Case: Detector Within Tolerance
        cout<<"This is found to be within Tolerance (" << aSetup.fUniformityTolerance << "); CHAMBER PASSES!!!\n";
        cout<<"You should Investigate the output data file\n";
    } //End Case: Detector Within Tolerance
    else{ //Case: Detector OUTSIDE Tolerance
        cout<<"This is found to be OUTSIDE Tolerance (" << aSetup.fUniformityTolerance << "); CHAMBER FAILS!!!\n";
        cout<<"You should Investigate the output data file\n";
    } //End Case: Detector OUTSIDE Tolerance
    
    return;
}*/ //check AnalyzeResponseUniformity::Uniformity

void AnalyzeResponseUniformity::calcStatistics(SummaryStatistics &inputStatObs, std::multiset<float> &mset_fInputObs, string strObsName){
    //Variable Declaration;
    std::vector<float> vec_fInputObsVariance;
    
    std::multiset<float>::iterator iterQ1 = mset_fInputObs.begin();
    std::multiset<float>::iterator iterQ2 = mset_fInputObs.begin();
    std::multiset<float>::iterator iterQ3 = mset_fInputObs.begin();

    //Determine max, min, & mean
    inputStatObs.fMax   = *mset_fInputObs.rbegin();   //Last member of the multiset
    inputStatObs.fMin   = *mset_fInputObs.begin();    //First member of the multiset
    inputStatObs.fMean  = std::accumulate( mset_fInputObs.begin(), mset_fInputObs.end(), 0. ) / mset_fInputObs.size();
    
    //Determine standard deviation
    float fAvg = inputStatObs.fMean;
    vec_fInputObsVariance.resize( mset_fInputObs.size() );
    std::transform(mset_fInputObs.begin(), mset_fInputObs.end(), vec_fInputObsVariance.begin(), [fAvg](float x) { return x - fAvg; });
    inputStatObs.fStdDev = std::sqrt( std::inner_product(vec_fInputObsVariance.begin(), vec_fInputObsVariance.end(), vec_fInputObsVariance.begin(), 0.0) / vec_fInputObsVariance.size() );
    
    //Determine Q1, Q2, & Q3
    std::advance(iterQ1, (int)std::ceil( mset_fInputObs.size() * 0.25 ) );  inputStatObs.fQ1 = *iterQ1;
    std::advance(iterQ2, (int)std::ceil( mset_fInputObs.size() * 0.50 ) );  inputStatObs.fQ2 = *iterQ2;
    std::advance(iterQ3, (int)std::ceil( mset_fInputObs.size() * 0.75 ) );  inputStatObs.fQ3 = *iterQ3;
    
    //Determine IQR
    inputStatObs.fIQR = inputStatObs.fQ3 - inputStatObs.fQ1;
    
    //Make distribution
    inputStatObs.hDist = std::make_shared<TH1F>( TH1F( getNameByIndex(-1, -1, -1, "h", strObsName + "Dataset" ).c_str(), "", 40, inputStatObs.fMean - 5. * inputStatObs.fStdDev, inputStatObs.fMean + 5. * inputStatObs.fStdDev) );
    inputStatObs.hDist->Sumw2();
    inputStatObs.hDist->GetXaxis()->SetTitle( strObsName.c_str() );
    inputStatObs.hDist->GetYaxis()->SetTitle( "N" );
    
    //Fill distribution & determine all outliers
    float fLowerBound = inputStatObs.fQ1 - 1.5 * inputStatObs.fIQR;
    float fUpperBound = inputStatObs.fQ3 + 1.5 * inputStatObs.fIQR;
    inRange inDataRange(fLowerBound, fUpperBound);
    
    inputStatObs.mset_fOutliers = mset_fInputObs;
    for (auto iterSet = inputStatObs.mset_fOutliers.begin(); iterSet != inputStatObs.mset_fOutliers.end(); ++iterSet) { //Loop Over input multiset
        inputStatObs.hDist->Fill( (*iterSet) );
        
        if (inDataRange( (*iterSet) ) ) {
            iterSet = inputStatObs.mset_fOutliers.erase(iterSet);
        }
    } //End Loop Over input multiset
    
    //Fit distribution
    inputStatObs.fitDist = std::make_shared<TF1>( TF1( getNameByIndex(-1, -1, -1, "fit", strObsName + "Dataset" ).c_str(), "gaus(0)", inputStatObs.fMean - 5. * inputStatObs.fStdDev, inputStatObs.fMean + 5. * inputStatObs.fStdDev) );
    inputStatObs.fitDist->SetParameter(0, inputStatObs.hDist->GetBinContent( inputStatObs.hDist->GetMaximumBin() ) );
    inputStatObs.fitDist->SetParameter(1, inputStatObs.hDist->GetMean() );
    inputStatObs.fitDist->SetParameter(2, inputStatObs.hDist->GetRMS() );
    
    inputStatObs.hDist->Fit(inputStatObs.fitDist.get(),"QM","", inputStatObs.fMean - 5. * inputStatObs.fStdDev, inputStatObs.fMean + 5. * inputStatObs.fStdDev );

    return;
} //End AnalyzeResponseUniformity::calcStatistics()

TF1 AnalyzeResponseUniformity::getFit(int iEta, int iPhi, int iSlice, HistoSetup & setupHisto, shared_ptr<TH1F> hInput, TSpectrum &specInput ){
    //Variable Declaration
    float fLimit_Max = setupHisto.fHisto_xUpper, fLimit_Min = setupHisto.fHisto_xLower;
    
    vector<string>::const_iterator iterVec_IGuess; //Iterator to use for setting initial guess of fit
    vector<float> vec_fFitRange;
    
    for (auto iterRange = aSetup.histoSetup_clustADC.vec_strFit_Range.begin(); iterRange != aSetup.histoSetup_clustADC.vec_strFit_Range.end(); ++iterRange) { //Loop Over Fit Range
        vec_fFitRange.push_back( getParsedInput( (*iterRange), hInput, specInput ) );
    } //End Loop Over Fit Range
    
    if (vec_fFitRange.size() > 1) {
        fLimit_Min = (*std::min_element(vec_fFitRange.begin(), vec_fFitRange.end() ) );
        fLimit_Max = (*std::max_element(vec_fFitRange.begin(), vec_fFitRange.end() ) );
    }
    
    TF1 ret_Func( getNameByIndex(iEta, iPhi, iSlice, "fit", setupHisto.strHisto_Name).c_str(), setupHisto.strFit_Formula.c_str(), fLimit_Min, fLimit_Max);
    
    //Check to see if the number of parameters in the TF1 meets the expectation
    if ( ret_Func.GetNpar() < setupHisto.vec_strFit_ParamIGuess.size() || ret_Func.GetNpar() < setupHisto.vec_strFit_ParamLimit_Min.size() || ret_Func.GetNpar() < setupHisto.vec_strFit_ParamLimit_Max.size() ) { //Case: Set points for initial parameters do not meet expectations

        printClassMethodMsg("AnalyzeResponseUniformity","getFit","Error! Number of Parameters in Function Less Than Requested Initial Guess Parameters!");
        printClassMethodMsg("AnalyzeResponseUniformity","getFit", ("\tNum Parameter: " + getString( ret_Func.GetNpar() ) ).c_str() );
        printClassMethodMsg("AnalyzeResponseUniformity","getFit", ("\tNum Initial Guesses: " + getString( setupHisto.vec_strFit_ParamIGuess.size() ) ).c_str() );
        printClassMethodMsg("AnalyzeResponseUniformity","getFit", ("\tNum Initial Guess Limits (Min): " + getString( setupHisto.vec_strFit_ParamLimit_Min.size() ) ).c_str() );
        printClassMethodMsg("AnalyzeResponseUniformity","getFit", ("\tNum Initial Guess Limits (Max): " + getString( setupHisto.vec_strFit_ParamLimit_Max.size() ) ).c_str() );
        printClassMethodMsg("AnalyzeResponseUniformity","getFit", "No Initial Parameters Have Been Set! Please Cross-Check Input Analysis Config File" );
        
        return ret_Func;
    } //End Case: Set points for initial parameters do not meet expectations
    
    //Set Fit Parameters - Initial Value
    //------------------------------------------------------
    //Keywords are defined in vec_strSupportedKeywords
    for (int i=0; i<setupHisto.vec_strFit_ParamIGuess.size(); ++i) { //Loop over parameters - Initial Guess
        ret_Func.SetParameter(i, getParsedInput( setupHisto.vec_strFit_ParamIGuess[i], hInput, specInput) );
    } //End Loop over parameters - Initial Guess
    
    //Set Fit Parameters - Boundaries
    //------------------------------------------------------
    if (setupHisto.vec_strFit_ParamLimit_Min.size() == setupHisto.vec_strFit_ParamLimit_Max.size() ) { //Check: Stored Parameter Limits Match

        //Here we use vec_strFit_ParamLimit_Min but we know it has the same number of parameters as vec_strFit_ParamLimit_Max
        //For each fit parameter, set the boundary
        for (int i=0; i<setupHisto.vec_strFit_ParamLimit_Min.size(); ++i) { //Loop over boundary parameters
            fLimit_Min = getParsedInput(setupHisto.vec_strFit_ParamLimit_Min[i], hInput, specInput);
            fLimit_Max = getParsedInput(setupHisto.vec_strFit_ParamLimit_Max[i], hInput, specInput);

            (fLimit_Max > fLimit_Min) ? ret_Func.SetParLimits(i, fLimit_Min, fLimit_Max ) : ret_Func.SetParLimits(i, fLimit_Max, fLimit_Min );
        } //End Loop over boundary parameters
    } //End Check: Stored Parameter Limits Match
    
    //Set Fit Parameters - Fixed?
    //------------------------------------------------------
    
    //Placeholder; maybe we add functionality in the future
    
    //Set Other Fit Data Members
    //------------------------------------------------------
    ret_Func.SetLineColor(kRed);
    ret_Func.SetLineWidth(3);
    
    //Return fit
    //------------------------------------------------------
    return ret_Func;
} //End AnalyzeResponseUniformity::getFit()

float AnalyzeResponseUniformity::getParsedInput(std::string &strInputExp, std::shared_ptr<TH1F> hInput, TSpectrum &specInput){
    //Variable Declaration
    map<string, float> map_key2Val;
    
    //Search the input expression for each of the supported keywords
    //Store these Keywords with their values
    for (int i=0; i < vec_strSupportedKeywords.size(); ++i) { //Loop Through Supported Keywords
        if ( strInputExp.find( vec_strSupportedKeywords[i] ) != std::string::npos ) { //Case: Keyword Found!
            map_key2Val[vec_strSupportedKeywords[i]] = getValByKeyword( vec_strSupportedKeywords[i], hInput, specInput );
        } //End Case: Keyword Found!
    } //End Loop Through Supported Keywords
    
    //Check if map_key2Val has any entries, if so user requested complex expression; parse!
    //If map_key2Val is empty, user has a numeric input; convert to float!
    if (map_key2Val.size() > 0) { //Case: Complex Expression!
        //Setup the expression parser
        symbol_table_t symbol_table;    //Stores the variables in expression and maps them to C++ objects
        expression_t expression;        //Stores the actual expression & the symbol table
        parser_t parser;                //Parses the information for evaluation
        
        //Load all found keywords into the symbol table
        for (auto iterMap = map_key2Val.begin(); iterMap != map_key2Val.end(); ++iterMap) {
            symbol_table.add_variable( (*iterMap).first, (*iterMap).second);
        }
        
        //Give the expression the variables it should have
        expression.register_symbol_table(symbol_table);
        
        //Compile the parsing
        parser.compile(strInputExp, expression);
        
        //Return value to the user
        return expression.value();
    } //End Case: Complex Expression!
    else{ //Case: Numeric Input
        return stofSafe( strInputExp );
    } //End Case: Numeric Input
} //End AnalyzeResponseUniformity::getParsedInput()

TGraphErrors AnalyzeResponseUniformity::getGraph(int iEta, int iPhi, HistoSetup & setupHisto){
    //Variable Declaration
    string strPrefix = "g";
    string strName = getNameByIndex(iEta, iPhi, -1, strPrefix, setupHisto.strHisto_Name);
    
    //Graph Declaration
    TGraphErrors ret_Graph;
    
    //Set Histo Data Members
    ret_Graph.GetXaxis()->SetTitle( setupHisto.strHisto_Title_X.c_str() );
    
    return ret_Graph;
} //End AnalyzeResponseUniformity::getGraph()

//Returns a histogram whose parmeters match those defined in hte input HistoSetup object
TH1F AnalyzeResponseUniformity::getHistogram(int iEta, int iPhi, HistoSetup &setupHisto){
    //Variable Declaration
    string strPrefix = "h";
    string strName = getNameByIndex(iEta, iPhi, -1, strPrefix, setupHisto.strHisto_Name);
    
    //Histo Declaration
    TH1F ret_Histo(strName.c_str(), "", setupHisto.iHisto_nBins, setupHisto.fHisto_xLower, setupHisto.fHisto_xUpper );

    //Set Histo Data Members
    ret_Histo.SetXTitle( setupHisto.strHisto_Title_X.c_str() );
    ret_Histo.SetYTitle( setupHisto.strHisto_Title_Y.c_str() );
    
    ret_Histo.Sumw2();
    
    //Set Directory to the global directory
    ret_Histo.SetDirectory(gROOT);

    //Return Histogram
    return ret_Histo;
} //End AnalyzeResponseUniformity::getHistogram()

TH2F AnalyzeResponseUniformity::getHistogram2D(int iEta, int iPhi, HistoSetup &setupHisto_X, HistoSetup &setupHisto_Y){
    //Variable Declaration
    string strPrefix = "h";
    string strName = getNameByIndex(iEta, iPhi, -1, strPrefix, setupHisto_Y.strHisto_Name + "_v_" + setupHisto_X.strHisto_Name );
    
    //Histo Declaration
    TH2F ret_Histo(strName.c_str(), "", setupHisto_X.iHisto_nBins, setupHisto_X.fHisto_xLower, setupHisto_X.fHisto_xUpper, setupHisto_Y.iHisto_nBins, setupHisto_Y.fHisto_xLower, setupHisto_Y.fHisto_xUpper);
    
    //Set Histo Data Members
    ret_Histo.SetXTitle( setupHisto_X.strHisto_Title_X.c_str() );
    ret_Histo.SetYTitle( setupHisto_Y.strHisto_Title_X.c_str() );
    
    ret_Histo.Sumw2();
    
    //Set Directory to the global directory
    ret_Histo.SetDirectory(gROOT);
    
    //Return Histogram
    return ret_Histo;
} //End AnalyzeResponseUniformity::getHistogram2D

//Formats a given input string such that it follows the iEta, iPhi, iSlice naming convention
string AnalyzeResponseUniformity::getNameByIndex(int iEta, int iPhi, int iSlice, std::string & strInputPrefix, std::string & strInputName){
    //Variable Declaration
    string ret_Name = "";
    
    if (iSlice > -1) {
        ret_Name = strInputPrefix + "_iEta" + getString(iEta) + "iPhi" + getString(iPhi) + "Slice" + getString(iSlice) + "_" + strInputName;
    }
    else if (iPhi > -1){ //Case: Specific (iEta,iPhi) sector
        ret_Name = strInputPrefix + "_iEta" + getString(iEta) + "iPhi" + getString(iPhi) + "_" + strInputName;
    } //End Case: Specific (iEta,iPhi) sector
    else if (iEta > -1){
        ret_Name = strInputPrefix + "_iEta" + getString(iEta) + "_" + strInputName;
    }
    else{ //Case: iEta Sector, sum over sector's iPhi
        ret_Name = strInputPrefix + "_Summary_" + strInputName;
    } //End Case: iEta Sector, sum over sector's iPhi
    
    return ret_Name;
} //End AnalyzeResponseUniformity::getNameByIndex()

//Formats a given input string such that it follows the iEta, iPhi, iSlice naming convention
//Will take text written in quotes without requiring a variable
string AnalyzeResponseUniformity::getNameByIndex(int iEta, int iPhi, int iSlice, const std::string & strInputPrefix, const std::string & strInputName){
    //Variable Declaration
    string ret_Name = "";
    
    if (iSlice > -1) {
        ret_Name = strInputPrefix + "_iEta" + getString(iEta) + "iPhi" + getString(iPhi) + "Slice" + getString(iSlice) + "_" + strInputName;
    }
    else if (iPhi > -1){ //Case: Specific (iEta,iPhi) sector
        ret_Name = strInputPrefix + "_iEta" + getString(iEta) + "iPhi" + getString(iPhi) + "_" + strInputName;
    } //End Case: Specific (iEta,iPhi) sector
    else if (iEta > -1){
        ret_Name = strInputPrefix + "_iEta" + getString(iEta) + "_" + strInputName;
    }
    else{ //Case: iEta Sector, sum over sector's iPhi
        ret_Name = strInputPrefix + "_Summary_" + strInputName;
    } //End Case: iEta Sector, sum over sector's iPhi
    
    return ret_Name;
} //End AnalyzeResponseUniformity::getNameByIndex()

//Searches the input TF1 for a parameter with meaning given by strParam and stored in HistoSetup
//This parameter is then returned to the user
float AnalyzeResponseUniformity::getParam( shared_ptr<TF1> fitInput, HistoSetup & setupHisto, std::string strParam ){
    //Variable Declaration
    int iParamPos = -1;
    
    float ret_Val = -1;
    
    vector<string>::iterator iterParamMeaning = std::find(setupHisto.vec_strFit_ParamMeaning.begin(), setupHisto.vec_strFit_ParamMeaning.end(), strParam);
    
    if ( iterParamMeaning != setupHisto.vec_strFit_ParamMeaning.end() ) { //Case: Parameter Found!!!
        
        iParamPos = std::distance(setupHisto.vec_strFit_ParamMeaning.begin(), iterParamMeaning);
        
        ret_Val = fitInput->GetParameter(iParamPos);
    } //End Case: Parameter Found!!!
    else{ //Case: Parameter NOT Found
        printClassMethodMsg("AnalyzeResponseUniformity","getParam",("Error! - I did not find your requested parameter: " + strParam + "!\n").c_str() );
        printClassMethodMsg("AnalyzeResponseUniformity","getPeakPos","\tPlease Cross-check input analysis config file.\n");
    } //End Case: Parameter NOT Foun
    
    return ret_Val;
} //End AnalyzeResponseUniformity::getParam

//Searches the input TF1 for a parameter with meaning given by strParam and stored in HistoSetup
//The error on this parameter is then returned to the user
float AnalyzeResponseUniformity::getParamError( shared_ptr<TF1> fitInput, HistoSetup & setupHisto, std::string strParam ){
    //Variable Declaration
    int iParamPos = -1;
    
    float ret_Val = -1;
    
    vector<string>::iterator iterParamMeaning = std::find(setupHisto.vec_strFit_ParamMeaning.begin(), setupHisto.vec_strFit_ParamMeaning.end(), strParam);
    
    if ( iterParamMeaning != setupHisto.vec_strFit_ParamMeaning.end() ) { //Case: Parameter Found!!!
        
        iParamPos = std::distance(setupHisto.vec_strFit_ParamMeaning.begin(), iterParamMeaning);
        
        ret_Val = fitInput->GetParError(iParamPos);
    } //End Case: Parameter Found!!!
    else{ //Case: Parameter NOT Found
        printClassMethodMsg("AnalyzeResponseUniformity","getParamError",("Error! - I did not find your requested parameter: " + strParam + "!\n").c_str() );
        printClassMethodMsg("AnalyzeResponseUniformity","getParamError","\tPlease Cross-check input analysis config file.\n");
    } //End Case: Parameter NOT Foun
    
    return ret_Val;
} //End AnalyzeResponseUniformity::getParamError

//Given an input histogram and TSpectrum returns a numeric value based on the input keyword; supported keywords are "AMPLITUDE,MEAN,PEAK,SIGMA"
float AnalyzeResponseUniformity::getValByKeyword(string strInputKeyword, shared_ptr<TH1F> hInput, TSpectrum &specInput){
    
    //Try to automatically assign a value
    if ( 0 == strInputKeyword.compare("AMPLITUDE") ) { //Case: Histo Amplitude
        return hInput->GetBinContent( hInput->GetMaximumBin() );
    } //End Case: Histo Amplitude
    else if( 0 == strInputKeyword.compare("FWHM") ){ //Case: Full Width Half Max
        //Right now as estimate we just use the histo RMS
        return hInput->GetRMS();
    } //End Case: Full Width Half Max
    else if( 0 == strInputKeyword.compare("HWHM") ){ //Case: Half Width Half Max
        //Right now as estimate we just use half the histo RMS
        return 0.5 * hInput->GetRMS();
    } //End Case: Half Width Half Max
    else if (0 == strInputKeyword.compare("MEAN") ) { //Case: Histo Mean
        return hInput->GetMean();
    } //End Case: Histo Mean
    else if ( 0 == strInputKeyword.compare("PEAK") ){ //Case: Histo Peak
        Double_t *dPeakPos = specInput.GetPositionX();
        
        return dPeakPos[0];
    } //End Case: Histo Peak
    else if (0 == strInputKeyword.compare("SIGMA") ) { //Case: Histo RMS
        return hInput->GetRMS();
    } //End Case: Histo RMS
    else{ //Case: manual assignment
        printClassMethodMsg("AnalyzeResponseUniformity","getValByKeyword","Error! Input Keyword Not Recognized");
        printClassMethodMsg("AnalyzeResponseUniformity","getValByKeyword", ("\tGiven: " + strInputKeyword ).c_str() );
        printClassMethodMsg("AnalyzeResponseUniformity","getValByKeyword","\tRecognized Keywords:\n");
        
        for (int i=0; i < vec_strSupportedKeywords.size(); ++i) {
            printClassMethodMsg("AnalyzeResponseUniformity","getValByKeyword", vec_strSupportedKeywords[i].c_str() );
        }
        
        printClassMethodMsg("AnalyzeResponseUniformity","getValByKeyword","\tUndefined Behavior May Occur");
        
        return -1e12;
    } //End Case: manual assignment
} //End AnalyzeResponseUniformity::getValByKeyword()

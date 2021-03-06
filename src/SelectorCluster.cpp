//
//  SelectorCluster.cpp
//  
//
//  Created by Brian L Dorney on 28/01/16.
//
//

#include "SelectorCluster.h"

using std::cout;
using std::string;

using namespace Uniformity;

//Default Constructor
SelectorCluster::SelectorCluster(){
    
} //End Default Constructor

//Given an output ROOT file from AMORE (ROOTDATATYPE = CLUSTERS)
//Applies the cluster selection and stores those selected clusters in inputDet
//Input is a std::string storing the physical filename
void SelectorCluster::setClusters(std::string &strInputRootFileName, Uniformity::DetectorMPGD &inputDet){
    //Variable Declaration
    //int iFirstEvt = aSetupUniformity.iEvt_First;
    //int iNEvt = aSetupUniformity.iEvt_Total;
    
    //Int_t iClustMulti;  //I cry a little inside because of this
    //Int_t iClustPos_Y[3072];
	//Int_t iClustSize[3072];
	//Int_t iClustTimeBin[3072];
    
	//Float_t fClustPos_X[3072];
    //Float_t fClustADC[3072];
    
    //Cluster clust;
    
    TFile *file_ROOT = NULL;
    //TTree *tree_Clusters = NULL;
    
    //Open this run's root file
    //------------------------------------------------------
    file_ROOT = new TFile(strInputRootFileName.c_str(),"READ","",1);
    
    //Check to see if data file opened successfully, if so load the tree
    //------------------------------------------------------
    if ( !file_ROOT->IsOpen() || file_ROOT->IsZombie() ) { //Case: failed to load ROOT file
        perror( ("Uniformity::SelectorCluster::setClusters() - error while opening file: " + strInputRootFileName ).c_str() );
        Timing::printROOTFileStatus(file_ROOT);
        std::cout << "Exiting!!!\n";
        
        return;
    } //End Case: failed to load ROOT file
    
    //Simplied to just call the method below
    /*tree_Clusters = (TTree*) file_ROOT->Get("TCluster");
    
    if ( nullptr == tree_Clusters ) { //Case: failed to load TTree
        printClassMethodMsg("SelectorCluster","setClusters",("error while fetching: " + strInputRootFileName ).c_str() );
        printClassMethodMsg("SelectorCluster","setClusters","\tTree returns nullptr; Exiting!!!");

	return;
    } //End Case: failed to load TTree
    
    //Initialize Tree Branch Address to retrieve the cluster information
    //------------------------------------------------------
    tree_Clusters->SetBranchAddress("nclust", &iClustMulti);
    tree_Clusters->SetBranchAddress("clustPos",&fClustPos_X);
    tree_Clusters->SetBranchAddress("clustSize",&iClustSize);
    tree_Clusters->SetBranchAddress("clustADCs",&fClustADC);
    tree_Clusters->SetBranchAddress("clustTimebin",&iClustTimeBin);
    tree_Clusters->SetBranchAddress("planeID",&iClustPos_Y);
    
    if ( -1 == iNEvt ) { //Case: All Events
        iFirstEvt = 0;
        iNEvt = tree_Clusters->GetEntries();
    } //End Case: All Events
    else{ //Case: Event Range
        if ( iFirstEvt > tree_Clusters->GetEntries() ) { //Case: Incorrect Event Range, 1st Event Requested Beyond All Events
            printClassMethodMsg("SelectorCluster","setClusters", ("Error, First Event Requested as " + Timing::getString( aSetupUniformity.iEvt_First ) + " Greater Thant Total Number of Events " + Timing::getString( tree_Clusters->GetEntries() ) ).c_str() );
            printClassMethodMsg("SelectorCluster","setClusters", "Exiting!!!");
            return;
        } //End Case: Incorrect Event Range, 1st Event Requested Beyond All Events
        else if( (iFirstEvt + iNEvt) > tree_Clusters->GetEntries() ){
            iNEvt = tree_Clusters->GetEntries() - iFirstEvt;
        }
        else if( iFirstEvt < 0){
            iFirstEvt = 0;
        }
    } //End Case: Event Range
    
    //Get data event-by-event
    //------------------------------------------------------
    //for (int i=0; i < tree_Clusters->GetEntries(); ++i) {
    for (int i=iFirstEvt; i < iNEvt; ++i) {
        //Needed to implement a Hack
        //First check to make sure the cluster multiplicity is within the selection
        //Only then get the info on the clusters
        
        //Make sure we only read the number of clusters
        tree_Clusters->SetBranchStatus("*",0);
        tree_Clusters->SetBranchStatus("nclust",1);
        
        //Get the number of clusters
        tree_Clusters->GetEntry(i);

        //Output to the user some message that we are still running
        if (i % 1000 == 0) cout<<"Cluster Selection; " <<i<<" Events Analyzed\n";
        
        //If the event fails to pass the selection; skip it
        //---------------Event Selection---------------
        //Cut on number of clusters
        if ( !(aSetupUniformity.selClust.iCut_MultiMin < iClustMulti && iClustMulti < aSetupUniformity.selClust.iCut_MultiMax) ) continue;
        
        //Okay make sure we can read all branches
        tree_Clusters->SetBranchStatus("*",1);

        //Now get the remaining data
        tree_Clusters->GetEntry(i);

        //Loop Over the elements of the cluster array (yes it must be done like this due to how hte NTuple from AMORE is created)
        //For each element create a cluster, and check if it passes the selection
        for (int j=0; j < iClustMulti; ++j) { //Loop Over Number of Clusters
            //Set the cluster info
            clust.iPos_Y = iClustPos_Y[j];
            clust.fPos_X = fClustPos_X[j];
            
            clust.fADC = fClustADC[j];
            
            clust.iSize = iClustSize[j];
            
            clust.iTimeBin = iClustTimeBin[j];
            
            //If the cluster fails to pass the selection; skip it
            //---------------Cluster Selection---------------
            if ( !clusterPassesSelection(clust) ) continue;
            
            //If a cluster makes it here, store it in the detector
            inputDet.setCluster(clust);
        } //End Loop Over Number of Clusters
    } //End Loop Over "Events"
    */
    
    //Calling setClusters
    //------------------------------------------------------
    setClusters(file_ROOT, inputDet);
    
    //Close the Input ROOT File
    //------------------------------------------------------
    file_ROOT->Close();
    
    return;
} //End SelectorCluster::setClusters()

//Given an output ROOT file from AMORE (ROOTDATATYPE = CLUSTERS)
//Applies the cluster selection and stores those selected clusters in inputDet
//Input is a TFile *
void SelectorCluster::setClusters(TFile * file_InputRootFile, Uniformity::DetectorMPGD &inputDet){
    //Variable Declaration
    int iFirstEvt = aSetupUniformity.iEvt_First;
    int iNEvt = aSetupUniformity.iEvt_Total;
    
    Int_t iClustMulti;  //I cry a little inside because of this
    Int_t iClustPos_Y[3072];
    Int_t iClustSize[3072];
    Int_t iClustTimeBin[3072];
    
    Float_t fClustPos_X[3072];
    Float_t fClustADC[3072];
    
    Cluster clust;
    
    TTree *tree_Clusters = NULL;
    
    //Check to see if data file opened successfully, if so load the tree
    //------------------------------------------------------
    if ( !file_InputRootFile->IsOpen() || file_InputRootFile->IsZombie() ) { //Case: failed to load ROOT file
        perror( ("Uniformity::SelectorCluster::setClusters() - error while opening file: " + (string) file_InputRootFile->GetName() ).c_str() );
        Timing::printROOTFileStatus(file_InputRootFile);
        std::cout << "Exiting!!!\n";
        
        return;
    } //End Case: failed to load ROOT file
    
    tree_Clusters = (TTree*) file_InputRootFile->Get("TCluster");
    
    if ( nullptr == tree_Clusters ) { //Case: failed to load TTree
        printClassMethodMsg("SelectorCluster","setClusters",("error while fetching: " + (string) file_InputRootFile->GetName() ).c_str() );
        printClassMethodMsg("SelectorCluster","setClusters","\tTree returns nullptr; Exiting!!!");
        
        return;
    } //End Case: failed to load TTree
    
    //Initialize Tree Branch Address to retrieve the cluster information
    //------------------------------------------------------
    tree_Clusters->SetBranchAddress("nclust", &iClustMulti);
    tree_Clusters->SetBranchAddress("clustPos",&fClustPos_X);
    tree_Clusters->SetBranchAddress("clustSize",&iClustSize);
    tree_Clusters->SetBranchAddress("clustADCs",&fClustADC);
    tree_Clusters->SetBranchAddress("clustTimebin",&iClustTimeBin);
    tree_Clusters->SetBranchAddress("planeID",&iClustPos_Y);
    
    if ( -1 == iNEvt ) { //Case: All Events
        iFirstEvt = 0;
        iNEvt = tree_Clusters->GetEntries();
    } //End Case: All Events
    else{ //Case: Event Range
        if ( iFirstEvt > tree_Clusters->GetEntries() ) { //Case: Incorrect Event Range, 1st Event Requested Beyond All Events
            printClassMethodMsg("SelectorCluster","setClusters", ("Error, First Event Requested as " + Timing::getString( aSetupUniformity.iEvt_First ) + " Greater Thant Total Number of Events " + Timing::getString( tree_Clusters->GetEntries() ) ).c_str() );
            printClassMethodMsg("SelectorCluster","setClusters", "Exiting!!!");
            return;
        } //End Case: Incorrect Event Range, 1st Event Requested Beyond All Events
        else if( (iFirstEvt + iNEvt) > tree_Clusters->GetEntries() ){
            iNEvt = tree_Clusters->GetEntries() - iFirstEvt;
        }
        else if( iFirstEvt < 0){
            iFirstEvt = 0;
        }
    } //End Case: Event Range
    
    //Get data event-by-event
    //------------------------------------------------------
    //for (int i=0; i < tree_Clusters->GetEntries(); ++i) {
    for (int i=iFirstEvt; i < iNEvt; ++i) {
        //Needed to implement a Hack
        //First check to make sure the cluster multiplicity is within the selection
        //Only then get the info on the clusters
        
        //Make sure we only read the number of clusters
        tree_Clusters->SetBranchStatus("*",0);
        tree_Clusters->SetBranchStatus("nclust",1);
        
        //Get the number of clusters
        tree_Clusters->GetEntry(i);
        
        //Output to the user some message that we are still running
        if (i % 1000 == 0) cout<<"Cluster Selection; " <<i<<" Events Analyzed\n";
        
        //If the event fails to pass the selection; skip it
        //---------------Event Selection---------------
        //Cut on number of clusters
        if ( !(aSetupUniformity.selClust.iCut_MultiMin < iClustMulti && iClustMulti < aSetupUniformity.selClust.iCut_MultiMax) ) continue;
        
        //Okay make sure we can read all branches
        tree_Clusters->SetBranchStatus("*",1);
        
        //Now get the remaining data
        tree_Clusters->GetEntry(i);
        
        //Loop Over the elements of the cluster array (yes it must be done like this due to how hte NTuple from AMORE is created)
        //For each element create a cluster, and check if it passes the selection
        for (int j=0; j < iClustMulti; ++j) { //Loop Over Number of Clusters
            //Set the cluster info
            clust.iPos_Y = iClustPos_Y[j];
            clust.fPos_X = fClustPos_X[j];
            
            clust.fADC = fClustADC[j];
            
            clust.iSize = iClustSize[j];
            
            clust.iTimeBin = iClustTimeBin[j];
            
            //If the cluster fails to pass the selection; skip it
            //---------------Cluster Selection---------------
            if ( !clusterPassesSelection(clust) ) continue;
            
            //If a cluster makes it here, store it in the detector
            inputDet.setCluster(clust);
        } //End Loop Over Number of Clusters
    } //End Loop Over "Events"
    
    //Do not close the input TFile, it is used elsewhere
    
    return;
} //End SelectorCluster::setClusters()

//Check if Cluster Passes selection stored in aSetupUniformity? True -> Passes; False -> Fails
bool SelectorCluster::clusterPassesSelection(Cluster &inputClust){
    //Cluster Selection
    
    //Cluster with ADC below noise threshold?
	//cout<<"inputClust.fADC = " << inputClust.fADC << std::endl;
    if (inputClust.fADC < aSetupUniformity.selClust.iCut_ADCNoise){ return false; }
    
    //Cluster Size too small or too large?
    if (inputClust.iSize < aSetupUniformity.selClust.iCut_SizeMin){ return false; }
    if (inputClust.iSize > aSetupUniformity.selClust.iCut_SizeMax) {return false; }
    
    //Cluster Time too early or too late?
    if (inputClust.iTimeBin < aSetupUniformity.selClust.iCut_TimeMin){ return false; }
    if (inputClust.iTimeBin > aSetupUniformity.selClust.iCut_TimeMax) {return false; }
    
    //If we arrive here the cluster passes our selection; give true
    return true;
} //End clusterPassesSelection()

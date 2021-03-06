#include "WCReader.h"

#include <iostream>
#include <vector>
#include <map>
#include <iomanip>

#include "TNamed.h"
#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include "TLorentzVector.h"
#include "TDatabasePDG.h"

using namespace std;

WCReader::WCReader(){}

//----------------------------------------------------------------
WCReader::WCReader(const char* filename, const char* jsonFileName)
{
    dbPDG = new TDatabasePDG();
    mc_daughters = new std::vector<std::vector<int> >;  // daughters id of this track; vector
    thresh_KE = 1; // MeV

    rootFile = new TFile(filename);

    jsonFile.open(jsonFileName);
}

//----------------------------------------------------------------
WCReader::~WCReader()
{
    jsonFile.close();

    delete mc_daughters;

    rootFile->Close();
    delete rootFile;
}

//----------------------------------------------------------------
void WCReader::DumpRunInfo()
{
    int runNo=0, subRunNo=0, eventNo=0, detector=0;

    TTree *tr = (TTree*)rootFile->Get("Trun");
    if (tr) {
        tr->SetBranchAddress("runNo", &runNo);
        tr->SetBranchAddress("subRunNo", &subRunNo);
        tr->SetBranchAddress("eventNo", &eventNo);
        tr->SetBranchAddress("detector", &detector);
        tr->GetEntry(0);
    }

    jsonFile << fixed << setprecision(0);

    jsonFile << '"' << "runNo" << '"' << ":" << '"' << runNo << '"' << "," << endl;
    jsonFile << '"' << "subRunNo" << '"' << ":" << '"' << subRunNo << '"' << "," << endl;
    jsonFile << '"' << "eventNo" << '"' << ":" << '"' << eventNo << '"' << "," << endl;
    if (detector == 0) {
        jsonFile << '"' << "geom" << '"' << ":" << '"' << "uboone" << '"' << endl;
    }
    else if (detector == 1) {
        jsonFile << '"' << "geom" << '"' << ":" << '"' << "dune35t" << '"' << endl;
    }
    else if (detector == 2) {
        jsonFile << '"' << "geom" << '"' << ":" << '"' << "protodune" << '"' << endl;
    }
    else if (detector == 3) {
        jsonFile << '"' << "geom" << '"' << ":" << '"' << "dune10kt_workspace" << '"' << endl;
    }
}

void WCReader::DumpDeadArea()
{
    int nPoints = 0;
    const int MAX_POINTS = 10;
    double y[MAX_POINTS], z[MAX_POINTS];

    TTree *t = (TTree*)rootFile->Get("T_bad");
    if (t) {
        t->SetBranchAddress("bad_npoints", &nPoints);
        t->SetBranchAddress("bad_y", &y);
        t->SetBranchAddress("bad_z", &z);
    }

    jsonFile << "[";
    jsonFile << fixed << setprecision(1);
    int nEntries = t->GetEntries();
    for (int i=0; i<nEntries; i++) {
        t->GetEntry(i);
        if (i>0) jsonFile << "," << endl;
        jsonFile << "[";
        for (int j=0; j<nPoints; j++) {
            if (j>0) jsonFile << ",";
            jsonFile << "[" << y[j] << ", " << z[j] << "]";
        }
        jsonFile << "]";
    }
    jsonFile << "]" << endl;
}

//----------------------------------------------------------------
void WCReader::DumpSpacePoints(TString option)
{
    double x=0, y=0, z=0, q=0, nq=1;
    vector<double> vx, vy, vz, vq, vnq;
    TTree * t = 0;
    int flag = 0;

    if (option == "truth" || option == "true") {
        t = (TTree*)rootFile->Get("T_true");
    }
    else if (option == "rec_simple" || option == "simple") {
        t = (TTree*)rootFile->Get("T_rec");
    }
    else if (option == "2psimple"){
        t = (TTree*)rootFile->Get("T_2p");
    }
    else if (option == "rec_charge_blob" || option == "charge") {
        t = (TTree*)rootFile->Get("T_rec_charge");
    }
    else if (option == "rec_charge_cell" || option == "mixed") {
        t = (TTree*)rootFile->Get("T_rec_charge_blob");
    }
    else if (option == "rec_pattern" || option == "pattern"){
      t = (TTree*)rootFile->Get("TC");
      flag = 1;
    }
    else {
        cout << "WARNING: Wrong option: " << option << endl;
    }

    if (t && flag ==0) {
        t->SetBranchAddress("x", &x);
        t->SetBranchAddress("y", &y);
        t->SetBranchAddress("z", &z);
        if (! option.Contains("simple") ) {
            t->SetBranchAddress("q", &q);
        }
        if (option.Contains("charge")) {
            t->SetBranchAddress("nq", &nq);
        }
        int nPoints = t->GetEntries();
        for (int i=0; i<nPoints; i++) {
            t->GetEntry(i);
            vx.push_back(x);
            vy.push_back(y);
            vz.push_back(z);
            vq.push_back(q);
            vnq.push_back(nq);
        }
    }else if (t && flag==1){
      t->SetBranchStatus("*",0);
      t->SetBranchStatus("xx",1);
      t->SetBranchStatus("yy",1);
      t->SetBranchStatus("zz",1);
      t->SetBranchStatus("charge",1);
      t->SetBranchAddress("xx",&x);
      t->SetBranchAddress("yy",&y);
      t->SetBranchAddress("zz",&z);
      t->SetBranchAddress("charge",&q);
      nq = 1; //hack for now
      
      int nPoints = t->GetEntries();
      for (int i=0; i<nPoints; i++) {
	t->GetEntry(i);
	vx.push_back(x);
	vy.push_back(y);
	vz.push_back(z);
	vq.push_back(q);
	vnq.push_back(nq);
      }
    }

    jsonFile << "{" << endl;

    jsonFile << fixed << setprecision(1);

    print_vector(jsonFile, vx, "x");
    print_vector(jsonFile, vy, "y");
    print_vector(jsonFile, vz, "z");

    jsonFile << fixed << setprecision(0);
    print_vector(jsonFile, vq, "q");
    print_vector(jsonFile, vnq, "nq");


    jsonFile << '"' << "type" << '"' << ":" << '"' << option << '"' << "," << endl;

    // always dump runinfo in the end
    DumpRunInfo();

    jsonFile << "}" << endl;

}

//----------------------------------------------------------------
void WCReader::DumpMC()
{
    TTree *t = (TTree*)rootFile->Get("TMC");

    t->SetBranchAddress("mc_Ntrack"       , &mc_Ntrack);
    t->SetBranchAddress("mc_id"           , &mc_id);
    t->SetBranchAddress("mc_pdg"          , &mc_pdg);
    t->SetBranchAddress("mc_process"      , &mc_process);
    t->SetBranchAddress("mc_mother"       , &mc_mother);
    t->SetBranchAddress("mc_daughters"    , &mc_daughters);
    t->SetBranchAddress("mc_startXYZT"    , &mc_startXYZT);
    t->SetBranchAddress("mc_endXYZT"      , &mc_endXYZT);
    t->SetBranchAddress("mc_startMomentum", &mc_startMomentum);
    t->SetBranchAddress("mc_endMomentum"  , &mc_endMomentum);

    if (t) {
        t->GetEntry(0);
        ProcessTracks();
        DumpMCJSON(jsonFile);
    }
}

//----------------------------------------------------------------
void WCReader::ProcessTracks()
{
    // map track id to track index in the array
    for (int i=0; i<mc_Ntrack; i++) {
        trackIndex[mc_id[i]] = i;
    }

    // in trackParents, trackChildren, trackSiblings vectors, store track index (not track id)
    for (int i=0; i<mc_Ntrack; i++) {
        // currently, parent size == 1;
        // for primary particle, parent id = 0;
        vector<int> parents;
        if ( !IsPrimary(i) ) {
            parents.push_back(trackIndex[mc_mother[i]]);
        }
        trackParents.push_back(parents); // primary track will have 0 parents

        vector<int> children;
        int nChildren = (*mc_daughters).at(i).size();
        for (int j=0; j<nChildren; j++) {
            children.push_back(trackIndex[(*mc_daughters).at(i).at(j)]);
        }
        trackChildren.push_back(children);

    }

    // siblings
    for (int i=0; i<mc_Ntrack; i++) {
        vector<int> siblings;
        if ( IsPrimary(i) ) {
            for (int j=0; j<mc_Ntrack; j++) {
                if( IsPrimary(j) ) {
                    siblings.push_back(j);
                }
            }
        }
        else {
            // siblings are simply children of the mother
            int mother = trackIndex[mc_mother[i]];
            int nSiblings = trackChildren.at(mother).size();
            for (int j=0; j<nSiblings; j++) {
                siblings.push_back(trackChildren.at(mother).at(j));
            }
        }
        trackSiblings.push_back(siblings);
    }

}

//----------------------------------------------------------------
bool WCReader::DumpMCJSON(int id, ostream& out)
{
    int i = trackIndex[id];
    if (!KeepMC(i)) return false;

    int e = KE(mc_startMomentum[i])*1000;

    int nDaughter = (*mc_daughters).at(i).size();
    vector<int> saved_daughters;
    for (int j=0; j<nDaughter; j++) {
        int daughter_id = (*mc_daughters).at(i).at(j);
        // int e_daughter = KE(mc_startMomentum[ trackIndex[daughter_id] ])*1000;
        // if (e_daughter >= thresh_KE) {
        if ( KeepMC(trackIndex[daughter_id]) ) {
            saved_daughters.push_back(daughter_id);
        }
    }

    out << fixed << setprecision(1);
    out << "{";

    out << "\"id\":" << id << ",";
    out << "\"text\":" << "\"" << PDGName(mc_pdg[i]) << "  " << e << " MeV\",";
    out << "\"data\":{";
    out << "\"start\":[" << mc_startXYZT[i][0] << ", " <<  mc_startXYZT[i][1] << ", " << mc_startXYZT[i][2] << "],";
    out << "\"end\":[" << mc_endXYZT[i][0] << ", " <<  mc_endXYZT[i][1] << ", " << mc_endXYZT[i][2] << "]";
    out << "},";
    out << "\"children\":[";
    int nSavedDaughter = saved_daughters.size();
    if (nSavedDaughter == 0) {
        out << "],";
        out << "\"icon\":" << "\"jstree-file\"";
        out << "}";
        return true;
    }
    else {
        for (int j=0; j<nSavedDaughter; j++) {
            DumpMCJSON(saved_daughters.at(j), out);
            if (j!=nSavedDaughter-1) {
                out << ",";
            }
        }
        out << "]";
        out << "}";
        return true;
    }
}

//----------------------------------------------------------------
void WCReader::DumpMCJSON(ostream& out)
{
    out << "[";
    vector<int> primaries;
    for (int i=0; i<mc_Ntrack; i++) {
        if (IsPrimary(i)) {
            // int e = KE(mc_startMomentum[i])*1000;
            // if (e<thresh_KE) continue;
            if (KeepMC(i)) {
                primaries.push_back(i);
            }
        }
    }
    int size = primaries.size();
    // cout << size << endl;
    for (int i=0; i<size; i++) {
        if (DumpMCJSON(mc_id[primaries[i]], out) && i!=size-1) {
            out << ", ";
        }
    }

    out << "]";
}

//----------------------------------------------------------------
bool WCReader::KeepMC(int i)
{
    double e = KE(mc_startMomentum[i])*1000;
    double thresh_KE_em = 5.; // MeV
    double thresh_KE_np = 10; // MeV
    if (mc_pdg[i]==22 || mc_pdg[i]==11 || mc_pdg[i]==-11) {
        if (e>=thresh_KE_em) return true;
        else return false;
    }
    else if (mc_pdg[i]==2112 || mc_pdg[i]==2212 || mc_pdg[i]>1e9) {
        if (e>=thresh_KE_np) return true;
        else return false;
    }
    return true;
}

//----------------------------------------------------------------
double WCReader::KE(float* momentum)
{
    TLorentzVector particle(momentum);
    return particle.E()-particle.M();
}

//----------------------------------------------------------------
TString WCReader::PDGName(int pdg)
{
    TParticlePDG *p = dbPDG->GetParticle(pdg);
    if (p == 0) {
        if (pdg>1e9) {
            int z = (pdg - 1e9) / 10000;
            int a = (pdg - 1e9 - z*1e4) / 10;
            TString name;
            if (z == 18) name = "Ar";

            else if (z == 17) name = "Cl";
            else if (z == 19) name = "Ca";
            else if (z == 16) name = "S";
            else if (z == 15) name = "P";
            else if (z == 14) name = "Si";
            else if (z == 1) name = "H";
            else if (z == 2) name = "He";

            else return pdg;
            return Form("%s-%i", name.Data(), a);
        }
        return pdg;
    }
    else {
        return p->GetName();
    }
}

//----------------------------------------------------------------
void WCReader::print_vector(ostream& out, vector<double>& v, TString desc, bool end)
{
    int N = v.size();

    out << '"' << desc << '"' << ":[";
    for (int i=0; i<N; i++) {
        out << v[i];
        if (i!=N-1) {
            out << ",";
        }
    }
    out << "]";
    if (!end) out << ",";
    out << endl;
}

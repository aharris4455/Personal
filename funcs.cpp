#include "funcs.hpp"

static auto lastClearTime = chrono::steady_clock::now();
const chrono::minutes CLEAR_INTERVAL(10);

string trimNonNumeric(const string &str){
    //Trim non numeric chars from the passed string
    string trimmed = str;
    while(!trimmed.empty() && !isdigit(trimmed.back())){
        trimmed.pop_back();
    }
    return trimmed;
}

double convertToDecimal(double degMin, char hemisphere){
    /*Converts input from degree minutes to decimal degrees
        degMin = degrees in minutes
        hemisphere = the hemisphere where the gps device is (essentially 
        used to define if positive of negative)*/
    int deg = static_cast<int>(degMin/100); //gets 
    double mins = degMin - (deg * 100);
    double decimalDegs = deg + (mins / 60);
    if (hemisphere == 'S' || hemisphere == 'W') {
        decimalDegs = -decimalDegs;
    }
    return decimalDegs;
}

void updateCoords(double lat, double longi){
    //Updates the coordinaites in coords.txt
    ofstream file("coords.txt");
    if(file.is_open()){
        file << lat << "," << longi << endl;
        file.close();
    }
    else{
        printf("Unabel to open file\n");
    }
    
}

void updateSatInfo(int PRN, int ELE, int AZI, int SNR){
    //Updates sattilite info in satInfo.txt, used for identifiying
    //what sattilites are above
    ofstream file;
    auto now = chrono::steady_clock::now();
    if(now - lastClearTime > CLEAR_INTERVAL){
        file.open("satinfo.txt", ios::trunc);
        file.close();
        lastClearTime = now;
    }
    file.open("satinfo.txt",ios::app);
    if(file.is_open()){
        int MAX_SNR = 100; //A hardcoded value to compensate for a parsing error yet to be fixed
        if(SNR > MAX_SNR){
            SNR = MAX_SNR;
            file << PRN << "," << ELE << "," << AZI << "," << SNR << endl;
        }
        else{
            file << PRN << "," << ELE << "," << AZI << "," << SNR << endl;
        }         
    }
    else{
        printf("Unabel to open file satInfo.txt");
    }
    file.close();
}

vector<string> split(const char* buf, char delim) {
    //A splitting fucntion 
    vector<string> result;
    string current;
    while (*buf) {
        if (*buf == delim) {
            if (!current.empty()) {
                result.push_back(current);
                current.clear();
            }
        } else {
            current += *buf;
        }
        buf++;
    }
    if (!current.empty()) {
        result.push_back(current);
    }
    return result;
}

sentanceType getSentType(const string &setnID){
    //Determines the sentence type 
    if(setnID == "$GPGGA"){
        return GPGGA;
    }
    else if(setnID == "$GPGSV"){
        return GPGSV;
    }
    else{
        return UNKNOWN;
    }
}

pair<double, double> getCoords(const vector<string> &sentence){
    /*TODO: Make this return something to pass to the plotting 
    function*/  
    //parses NMEA sentence for location information
    //$GPGGA,145810.00,6500.99954,N,02531.90790,E,1,09,1.27,20.4,M,20.9,M,,*6C
    //Comma seperator, index 2 = N/S, Index 4 W/E 
    //Number of satilites tracked = index 7
    /*
    THis has to be modified to just return a vector of the sentence unuseable rn for anything else other than GPGGA
    */
    double lat = 0.0;
    double longi = 0.0; 
    //Kinda redundant now that a switch handles the outer case before the call 
    if(!sentence.empty() && sentence[0] == "$GPGGA" && sentence.size() > 5){
        //printf("Coords: %s N, %s E\n", sentence[2].c_str(), sentence[4].c_str());
        double latValue = stod(sentence[2]);
        char latHemi = sentence[3][0];
        lat = convertToDecimal(latValue, latHemi); //Converts to decimal degs
        
        double longValue = stod(sentence[4]);
        char longHemi = sentence[5][0];
        longi = convertToDecimal(longValue, longHemi);

    }
    return make_pair(lat, longi);
}

map<int, tuple<int, int, int> > getSatInfo(const vector<string> &sentance){
    //Gets info of sattilites by parseing NMEA sentences
    map<int, tuple<int,int,int> > satData;
    size_t len = sentance.size();
    vector<vector<string> > grouped; //Remember stoi !!!!
    for(int i = 0; i < len; i += 4){
        vector<string> group;
        auto endIT = ((i + 4 < len) ? sentance.begin() + i + 4 : sentance.end());
        group.insert(group.end(), sentance.begin() + i, endIT);
        grouped.push_back(group);
    }
    for(const auto &group : grouped){
        if(group == grouped[0]){
            continue;
        }
        else{
            string PRN_STR = trimNonNumeric(group[0]);
            string ELE_STR = trimNonNumeric(group[1]);
            string AZI_STR = trimNonNumeric(group[2]);
            string SNR_STR = trimNonNumeric(group[3]);
            try{
                int PRN = stoi(PRN_STR);
                int ELE = stoi(ELE_STR);
                int AZI = stoi(AZI_STR);
                int SNR = stoi(SNR_STR);
                satData.insert({PRN, make_tuple(ELE,AZI,SNR)});
            }
            catch(const invalid_argument &e){
                cerr << "Invalid argument, No int for you" << endl;
            }
            catch(const out_of_range){
                cerr << "Out of range lol" << endl;
            }
        }
    }
    return satData; 
}
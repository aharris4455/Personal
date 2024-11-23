#include "funcs.hpp"
#include <fcntl.h>
#include <termios.h>

using namespace std;

int main(int argc, char *argv[]){
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <serial port> <baud rate>" << std::endl;
        return -1;
    }
    //Serial port setup
    int fd;
    char *portname = argv[1];
    char buf[1024];
    int n;
    int i;
    int count = 0;
    int baudrate = stoi(argv[2]);
    struct termios toptions;
    fd = open(portname, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        perror("serialport_init: Unable to open port ");
        return -1;
    }
    if (tcgetattr(fd, &toptions) < 0) {
        perror("serialport_init: Couldn't get term attributes");
        return -1;
    }
    speed_t brate = baudrate;
    switch(baudrate) {
        case 4800:   brate=B4800;   break;
        case 9600:   brate=B9600;   break;
        #if defined B14400
        case 14400:  brate=B14400;  break;
        #endif
        case 19200:  brate=B19200;  break;
        #if defined B28800
        case 28800:  brate=B28800;  break;
        #endif
        case 38400:  brate=B38400;  break;
        case 57600:  brate=B57600;  break;
        case 115200: brate=B115200; break;
    }
    cfsetispeed(&toptions, brate);
    cfsetospeed(&toptions, brate);
    // 8N1
    toptions.c_cflag &= ~PARENB;
    toptions.c_cflag &= ~CSTOPB;
    toptions.c_cflag &= ~CSIZE;
    toptions.c_cflag |= CS8;
    // no flow control
    toptions.c_cflag &= ~CRTSCTS;
    toptions.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
    toptions.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

    toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
    toptions.c_oflag &= ~OPOST; // make raw
    toptions.c_cc[VMIN]  = 0;
    toptions.c_cc[VTIME] = 20;

    if( tcsetattr(fd, TCSANOW, &toptions) < 0) {
        perror("init_serialport: Couldn't set term attributes");
        return -1;
    }
    //Start serial port read 
    string buffer;
    while (1) {
        n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = 0;
            buffer += buf;
            //printf("read %i bytes: %s", n, buf);
            size_t pos;
            while((pos = buffer.find('\n')) != std::string::npos){
                string line = buffer.substr(0, pos);
                buffer.erase(0, pos + 1);
                vector<string> sentence = split(buf, ',');
                string sentID = sentence[0];
                sentanceType sentType = getSentType(sentID);
                pair<double, double> coords;
                map<int, tuple<int, int, int> > satInfo;
                //MAybe different threads
                switch(sentType){
                    case GPGGA:
                        coords = getCoords(sentence);
                        if (coords.first != 0.0 && coords.second != 0.0) { // Ensure valid coords
                            updateCoords(coords.first, coords.second);
                            printf("Coords: %f N, %f E\n", coords.first, coords.second);
                        }
                        break;
                    case GPGSV:
                        satInfo = getSatInfo(sentence);
                        for(const auto &entry : satInfo){
                            updateSatInfo(entry.first, get<0>(entry.second), 
                                                       get<1>(entry.second),
                                                       get<2>(entry.second));
                                
                            cout << "PRN: " << entry.first
                                 << ", Elevation: " << get<0>(entry.second)
                                 << ", Azimuth: " << get<1>(entry.second)
                                 << ", SNR: " << get<2>(entry.second) << endl;
                        }
                        break;
                    case UNKNOWN:
                        break;
                }
                /*Determine where to send the split sentence
                Based on the first element either gpgga or gpgsv 
                Switch statment: case gpgga: getCoords & updateCoords
                                   case gpgva: getSatInfo & updateSatInfo
                
                I guess just add the function for parsing the GPGSV data right here 
                //printf("Coords: %f N, %f S\n", coords.first, coords.second);
                
                //usleep(1000000);*/ 
                
            }
            
        }
    }
    close(fd);
    return 0;
}
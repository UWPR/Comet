#include "mzParser.h"

using namespace std;

int main(int argc, char* argv[]){

	if(argc!=2) {
		cout << "USAGE: mzMLReader <mzML file>" << endl;
		exit(0);
	}

	BasicSpectrum s;
	MzParser sax(&s);
	sax.load(argv[1]);

	/*
	RAMPFILE* r;
	RAMPREAL* peaks;
	ramp_fileoffset_t rio;
	ramp_fileoffset_t* ro;
	int iLastScan;
	
	
	r=rampOpenFile(argv[1]);
	if(r==NULL) cout << "Error reading file!" << endl;
	rio=getIndexOffset(r);
	ro=readIndex(r,rio,&iLastScan);

	cout << (long)rio << " " << iLastScan << endl;
	cout << (long)ro[5] << endl;
	peaks=readPeaks(r,ro[5]);

	int n=0;
	while(peaks[n]>-0.1){
		//cout << peaks[n] << "\t" << peaks[n+1] << endl;
		n+=2;
	}

	cout << "Here" << endl;
	rampCloseFile(r);
	free(ro);
	cout << "Exiting" << endl;
	exit(0);
	*/
	char c='a';
	char str[256];
	int num;
	while(c!='x'){

		cout << "\nCurrent spectrum:" << endl;
		cout << "\tScan number: " << s.getScanNum() << endl;
		cout << "\tRetention Time: " << s.getRTime() << endl;
		cout << "\tMS Level: " << s.getMSLevel() << endl;
		cout << "\tNumber of Peaks: " << s.size() << endl;
		cout << "\nMenu:\n\t's' to grab a new spectrum\n\t'p' to show peaks\n\t'x' to exit" << endl;
		cout << "Please enter your choice: ";
		cin >> c;

		switch(c){
			case 'p':
				for(unsigned int i=0;i<s.size();i++){
					printf("%.6lf\t%.1lf\n",s[i].mz,s[i].intensity);
				}
				break;
			case 's':
				cout << "Enter a number from " << sax.lowScan() << " to " << sax.highScan() << ": ";
				cin >> str;
				num=(int)atoi(str);
				if(num<sax.lowScan() || num>sax.highScan()) {
					cout << "Bad number! BOOOOO!" << endl;
				} else {
					if(!sax.readSpectrum(num)) cout << "Spectrum number not in file." << endl;
				}
				break;
			case 'x':
				break;
			default:
				cout << "\nInvalid command!" << endl;
				break;
		}
	}


	return 0;
}
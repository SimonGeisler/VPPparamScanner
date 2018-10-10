#include "Runner.hpp"

using namespace std;

int main(int argc, char* argv[]){
	//Configuration files and argv vector
	vector<string> configfiles;
	vector <string> argvec;
	
	//Standard settings
	bool skipVpp=false;
	bool skipSPheno=false;
	bool skipResults=false;
	bool single=true;
	bool keepold=false;
	bool print=false;
	bool slhafirst=false;
	//Reading argv
	for(int i=1; i<argc; i++) argvec.push_back(argv[i]);
	for(auto it=argvec.begin();it!=argvec.end();it++) {
		if(it->find(".xml")!=string::npos) configfiles.push_back(*it);
		else if(*it == "-skipVpp") skipVpp=true;
		else if(*it == "-skipSPheno") skipSPheno=true;
		else if(*it == "-skipResults") skipResults=true;
		else if(*it == "-notsingle") single=false;
		else if(*it == "-keepold") keepold=true;
		else if(*it == "-print") print=true;
		else if(*it == "-slhafirst") slhafirst=true;
		else cout << "Could not recognize the argument " << *it << 
		". Available Arguments: path/config.xml (necessary, File needs to end with .xml), -skipSPheno (optional), -skipVpp (optional), -skipResults (optional) -notsingle (optional) -keepold (optional)" << endl;
	}
	umask(0);
	//Configuration
	if(configfiles.empty()){
		throw "Configfile.xml needed, pass it to the console arguments, for example: ./execfile ./config.xml" ;
		return 0;
	}
	try {
		for(auto cit = configfiles.begin(); cit !=configfiles.end();cit++)
		{
			std::shared_ptr<Configuration> c (new Configuration);
			ReadConfiguration(*cit, c.get(), single);
			//Now Checking the options given
			string resultsdirname("");
			if(!c->options.empty())
			{
				if(skipVpp && c->options.find(skipVpp) == string::npos) c->options.append("\nskipVpp\n"); //-skipVpp in console always has priority
				vector<string> tempoptions;
				boost::split(tempoptions,c->options,boost::is_any_of("\n"));
				for(auto itv : tempoptions) {
					if(itv.find("skipVpp") !=string::npos) skipVpp=true;
					if(itv.find("skipSPheno") !=string::npos) skipSPheno=true;
					if(itv.find("skipResults") !=string::npos) skipResults=true;
					if(itv.find("print") !=string::npos) print=true;
					if(itv.find("dirname=")!=string::npos) {
						resultsdirname=itv.substr(itv.find("=")+1);
					}
				}
			}
			//----
			if(print) c->print();
			string erronname = "ErroneousInputs";
			string erron = c->results + erronname;
			auto t = time(nullptr);
			auto tm = *localtime(&t);
			bool skipSPhtemp;
			if(slhafirst) {
				skipSPhtemp=skipSPheno;
				skipSPheno=true;
			}
			if(!skipSPheno)
			{
				//Erroneous SPheno Input Header
				ofstream ofs;
				ofs.open (erron, ofstream::out | ofstream::app);
				ofs << c->sphenoinput << " " << put_time(&tm, "%d-%m-%Y %H-%M-%S") << endl <<endl;
				ofs.close();
				//-----------------------------
				
				//SPheno,Vpp output generation
				clearoutputfolder(c->vppoutput,false);
				clearoutputfolder(c->sphenooutput,false); //Clear all old files
				iterateparameters(c.get(), (c->inputparameters).begin());
			}
			else 
			{
				if(slhafirst) {
					bool skipVpptemp = skipVpp;
					skipVpp=true; 
					iterateparameters(c.get(), (c->inputparameters).begin());
					skipVpp = skipVpptemp;
				}
				if(!skipVpp)
				{
					clearoutputfolder(c->vppoutput,false);
					auto filenames = Filenames(c->sphenooutput);
					for(auto it : filenames)
					{
						cout << it << endl;
						for(auto il: c->lhaparameters) 
						{ //first read all of them
							if(il.term =="") c->werte.push_back(make_pair(il.name, il.readlhavalue(c->sphenooutput + it)));
							if(il.values.size() >1) {
								string error = "Error: LHA Parameter " + il.name + " has more than one value for one parameter";
								throw error;
							}
						}
						for(auto il: c->lhaparameters) 
						{
							if(il.term != "") c->werte.push_back(make_pair(il.name,il.calculatevalue(c->werte)));
							if(il.values.size() >1) 
							{
								string error = "Error: LHA Parameter " + il.name + " has more than one value for one parameter";
								throw error;
							}
						}
						RunVpp(c.get(), it);
						for(auto itw=(c->werte).begin(); itw!=(c->werte).end(); itw++) 
						{
							bool nomatch=true;
							for(auto itc=(c->constants).begin(); itc!=(c->constants).end(); itc++)
								if(itw->first == itc->first) nomatch=false;
							if(nomatch)
							{
								(c->werte).erase(itw); 
								itw--; //Erasing gives the next element but the for-loop also increments so we need to go back
							}
						}
					}
				}
			}
			//Writing Results
			if(!skipResults) {
				Writeresults(c.get(),*cit);
				std::ostringstream oss;
				string looptree = (c->loop)?"_Loop":"_Tree";
				string dname = (resultsdirname.empty())?"":(resultsdirname + c->delimiter);
				oss << dname <<  std::put_time(&tm, "%d-%m-%Y_%H-%M");
				mkdir( (c->results + oss.str()).c_str(), 0777);
				rename((c->results + "Results_stable").c_str(), (c->results + oss.str() + "/" + "Results_stable" + looptree).c_str());
				rename((c->results + "Results_metastable").c_str(), (c->results + oss.str() + "/" + "Results_metastable" + looptree).c_str());
				rename((c->results + "ErroneousInputs").c_str(), (c->results + oss.str() + "/" + "ErroneousInputs" + looptree).c_str());
				std::ifstream  src(*cit, std::ios::binary);
				string configname(*cit);
				configname = configname.substr(configname.find_last_of("/")+1); //Extract the configname
				std::ofstream  dst(c->results + oss.str() + "/" + configname,   std::ios::binary);
				dst << src.rdbuf();
			}
			//Going back to standard settings (we need that if a configfile changes settings
			skipSPheno=false;
			skipResults=false;
			skipVpp=false;
			for(auto it=argvec.begin();it!=argvec.end();it++) {
				if(*it == "-skipVpp") skipVpp=true;
				else if(*it == "-skipSPheno") skipSPheno=true;
				else if(*it == "-skipResults") skipResults=true;
			}
		}
	}
	catch(string error)
	{
		cout << "An error has occured:" << endl;
		cout << error << endl;
		return 1;
	}
	return 0;
}

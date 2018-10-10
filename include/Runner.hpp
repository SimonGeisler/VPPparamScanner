#ifndef RUNNER_HPP
#define RUNNER_HPP

#include "tools.hpp"

#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <random>
#include <memory>
#include <errno.h>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <map>
#include <utility>
#include <cstdlib>
#include <regex>

#include <boost/foreach.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

#include "exprtk.hpp"

using namespace std;
class Parameter{
	public:
	string term;
	vector<double> values;
	public:
	string name;
	vector<int> block;
	string blockname;
	double calculatevalue(vector<pair<string, double> > params) {
		exprtk::symbol_table<double> symbol_table;
		for(auto itw = params.begin(); itw != params.end();itw++)
		{
			symbol_table.add_variable(itw->first,itw->second);
		}
		exprtk::expression<double> expression;
		exprtk::parser<double> parser;
		expression.register_symbol_table(symbol_table);
		if(!parser.compile(this->term,expression))
		{
			stringstream error;
			error << "Parsing error of parameter " << this->name << " with the term " << this->term << endl;
			throw error.str();
		}
		this->values.clear(); //A calculated parameter always only contains one value, so we clear it here instead of somewhere else.
		this->values.push_back(expression.value());
		return expression.value();
	}
};

class InputParameter : public Parameter {
	public:
	InputParameter(string inpname, vector<int> inpblock, int stepsize, double startval, double endval, string inpblockname) //Start and Endvals Input parameter
	{
		this->name = inpname;
		this->blockname = inpblockname;
		if(!inpblock.empty()) this->block = inpblock;
		this->term = "";
		if(stepsize >0) for(int i = 0; i<=stepsize; i++)
			this->values.push_back(startval + i*(endval - startval)/(stepsize));
		else this->values.push_back(startval);
	}
	InputParameter(string inpname, vector<int> inpblock, string inpblockname) { //If Values are only added via addvalue(double val) in <values>...</values>
		this->name = inpname;
		this->blockname = inpblockname;
		if(!inpblock.empty()) this->block = inpblock;
	}
	InputParameter(string inpname, vector<int> inpblock, string inpterm, string inpblockname){ //calculated input parameters
		this->name = inpname;
		if(!inpblock.empty()) this->blockname = inpblockname;
		this->block = inpblock;
		this->term = inpterm;
		if(term==""){
			stringstream error;
			error << "Error, the parameter " << this->name << " needs a term to calculate" << endl;
			throw error.str();
		}
	}
	void addvalue(double val) { this->values.push_back(val); }
};

class LHAParameter : public Parameter {
	public:
	LHAParameter(string inpname, string inpblockname, vector<int> inpblock){
		this->name = inpname;
		if(!inpblock.empty()) this->blockname = inpblockname;
		else{
			stringstream error;
			error << "The parameter " << this->name << " has no block number defined"<< endl;
			throw error.str();
		}
		this->block = inpblock;
		this->term = ""; 
	}
	LHAParameter(string inpname, string inpterm){ 
		this->name = inpname;
		this->term = inpterm;
		if(term=="") {
			stringstream error;
			error << "Error, the parameter " << this->name << " needs a term to calculate" << endl;
			throw error.str();
		}
	}
	double readlhavalue(string filename) {
		this->values.clear(); //Clearing values since its only one value anyways
		ifstream t(filename.c_str());
		string container((istreambuf_iterator<char>(t)), istreambuf_iterator<char>()); 
		stringstream cont(container);
		t.close();
		string tempstring(""); 
		size_t startpos = container.find("Block " + this->blockname + " ");
		if(startpos == string::npos) startpos = container.find("BLOCK " + this->blockname + " ");//Lets Split container by blocks, First find beginning of Block container since we search for the next one in the next step
		if(startpos == string::npos) startpos = container.find("Block " + this->blockname + "\n");
		if(startpos == string::npos) startpos = container.find("BLOCK " + this->blockname + "\n");

		if(startpos != string::npos) tempstring = container.substr(startpos+5); //We dont want to have "Block" in the 
		if(startpos == string::npos) {
			stringstream error;
			error << "Could not find a Block named " << this->blockname ;
			throw error.str();
		}
		size_t pos = tempstring.find("Block");
		tempstring = (pos != string::npos)?tempstring.substr(0,pos-1):tempstring.substr(0); //Substring until next LHABlock
		string pattern("");
		for(auto itb : this->block) pattern.append(to_string(itb) + "\\s+");
		 //Now we just need to read the value and we're done.
		pattern.append( "(-?[0-9]+(?:\\.[0-9]+[eE][+-][0-9]+)?)\\s*[#]\\s*[0-9a-zA-Z_^]+" );
		regex rgx(pattern);
		smatch match;
		if (regex_search(tempstring, match, rgx))
		{
			(this->values).push_back(stod(match[1]));
			return stod(match[1]);
		}
		else{
			stringstream error;
			error << "No match for " << this->blockname << " ";
			for(auto itb : this->block) error << itb << " ";
			error << endl;
			throw error.str();
		}
	}
};

struct Configuration{
	string spheno;
	string sphenoinput;
	string sphenooutput;
	string vppoutput;
	string results;
	string delimiter;
	string VppExec;
	string configfilename;
	string options;
	int precision;
	vector<InputParameter> inputparameters;
	vector<string> inputblocks;
	vector<LHAParameter> lhaparameters;
	vector<string> paramconditions;
	vector<string> lhaconditions;
	
	vector<pair <string,double> > werte;
	vector<pair<string,double> > constants;
	bool loop; //Calculate Loop Masses?
	void print() {
		cout << "SPheno " << spheno << " sphenoinput, output " << sphenoinput << " , " << sphenooutput <<endl;
		cout << "Vevacious " << vppoutput << " " << VppExec << " " << results << endl;
		cout << "Inputparameters:" << endl;
		for (auto iti : inputparameters) {
			cout << iti.name << " " << iti.blockname << " ";
			for(auto itb : iti.block) cout << itb << " ";
			cout << endl;
			for(auto itv: iti.values )
				cout << itv << " ";
			cout << endl;
			cout << "Inputblocks " << endl;
			for(auto it : inputblocks) cout << it << " ";
			cout <<endl;
		}
		cout << endl;
	}
};
bool Checkconditions(Configuration *c, bool lha=false)
{
	exprtk::symbol_table<double> symbol_table;
	for(auto itw = (c->werte).begin(); itw != (c->werte).end();itw++)
	{
		symbol_table.add_variable(itw->first,itw->second); //Register all parameters
	}
	if(c->lhaconditions.empty() && lha || !lha && c->paramconditions.empty()) return true;
	for( auto itc = (lha)?(c->lhaconditions).begin():(c->paramconditions).begin();(lha)?itc!=(c->lhaconditions).end():itc!=(c->paramconditions).end();itc++)
	{
		exprtk::expression<double> expression;
		exprtk::parser<double> parser;
		expression.register_symbol_table(symbol_table);
		if(!parser.compile(*itc,expression))
		{
			stringstream error;
			error << "Parsing error of condition " << *itc << endl;
			throw error.str();
		}
		if(expression.value()==0) return false;
	}
	return true; //All went well
}

int RunVpp(Configuration *c, string file)
{
	editXML(c->configfilename, "VevaciousPlusPlusMainInput.SingleParameterPoint.RunPointInput", c->sphenooutput + file);
	editXML(c->configfilename, "VevaciousPlusPlusMainInput.SingleParameterPoint.OutputFilename", c->vppoutput + file + ".vout");
	//Run V++
	int error = system((c->VppExec + " " + c->configfilename).c_str());
	if(error!=-1) {
		//Appending all the values as XML to v++ output
		string xmltag("");
		for(auto itw  : c->werte) xmltag.append(itw.first + "=" + to_string(itw.second) + "\n");
		boost::trim(xmltag);
		ifstream ifs(c->vppoutput + file + ".vout");
		if(ifs.good()) //if there's no file we have V++ related error
		{
			ofstream ofs(c->vppoutput + file + ".vout", ofstream::app);
			ofs << endl;
			ofs << "<parameter>" << endl;
			ofs << xmltag << endl << "</parameter>";
			ofs.close();
		}
		else error=-1; //No file -> Error, but not systemcall related
	}
	else {
		cout << "Error running Vev++. Check the SPheno output folder." << endl;
		string stringbuilder("");
		for(auto it = (c->werte).begin(); it != (c->werte).end(); it++)
		{
			ostringstream wert;
			wert <<fixed <<setprecision(c->precision) << scientific << it->second;
			stringbuilder += " " + it->first + "=" + wert.str();
        }
		ofstream ofs;
		ofs.open (c->results + "ErroneousInputs", ofstream::out | ofstream::app);
		ofs << "V++ Erroneous with file " <<  file << endl;
		ofs.close();
	}
	return error;
}

int RunSPheno(Configuration *c)
{
	string outfile = c->sphenooutput + "R";
	string systemcommand = "sudo " + c->spheno + " " + c->sphenoinput + " " + outfile;
    int error = system((systemcommand).c_str()); 
	ifstream iff (outfile);
	string stringbuilder("");
	if ( !iff.good() ) //If SPheno didnt create a file
	{
		for(auto it = (c->werte).begin(); it != (c->werte).end(); it++)
		{
			ostringstream wert;
			wert <<fixed <<setprecision(c->precision) << scientific << it->second;
			stringbuilder += " " + it->first + "=" + wert.str();
        }
		ofstream ofs(c->results + "ErroneousInputs", ofstream::out | ofstream::app);
		ofs << " SPheno error with input: " << stringbuilder << endl;
		ofs.close();
		return 0; //We dont want to throw an exception because we already noted the parameters in ErroneousInputs
	}
	for(auto il: c->lhaparameters) { //first read all of them
		if(il.term =="") c->werte.push_back(make_pair(il.name, il.readlhavalue(outfile)));
		if(il.values.size() >1) {
			string error = "Error: LHA Parameter " + il.name + " has more than one value for one parameter";
			throw error;
		}
	}
	for(auto il: c->lhaparameters) {//Then calculate the rest
		if(il.term != "") c->werte.push_back(make_pair(il.name,il.calculatevalue(c->werte)));
		if(il.values.size() >1) {
			string error = "Error: LHA Parameter " + il.name + " has more than one value for one parameter";
			throw error;
		}
	}
	//We Now check if the LHA lhaconditions are met and fill the c->werte with the calculated parameters
	if(Checkconditions(c,true))
	{
		stringbuilder += to_string(randnumber(0.3,1.337));
		stringbuilder += to_string(randnumber(0.3,1.337));
		stringbuilder += to_string(randnumber(0.3,1.337));
		stringbuilder = regex_replace(stringbuilder, regex("\\."), "");
		string renamefile = c->sphenooutput + stringbuilder;
		ifstream off(renamefile);
		while(off.good()) {// in the extremely rare case the file name already exists
			renamefile += randnumber(0,1);
			off.close();
			ifstream off(renamefile);
		}
		rename(outfile.c_str(),renamefile.c_str());
		//Running Vevacious++ now
		if(c->options.find("skipVpp")==string::npos) error = RunVpp(c,stringbuilder);
	}
	else
	{ //If calculated conditions are not met
		remove(outfile.c_str());
		for(auto it = (c->werte).begin(); it != (c->werte).end(); it++)
		{
			ostringstream wert;
			wert <<fixed <<setprecision(c->precision) << scientific << it->second;
			stringbuilder += c->delimiter + it->first + "=" + wert.str();
        }
		ofstream ofs;
		ofs.open (c->results + "ErroneousInputs", ofstream::out | ofstream::app);
		string errinp = stringbuilder;
		boost::replace_all(errinp, "_", " ");
		boost::replace_all(errinp, "=", " = ");
		ofs << "lhaconditions not met with input: " << errinp << endl;
		ofs.close();
	}
	return error;
}

void ChangeInputFile(Configuration *c){
	ifstream t((c->sphenoinput).c_str());
	if(!t.good()){
		string error("Could not open the input file for SPheno");
		throw error;
	}
	string container((std::istreambuf_iterator<char>(t)),
							std::istreambuf_iterator<char>()); //Load content
	stringstream cont(container);
	t.close();
    string segment;
    vector<string> lines;
    while(getline(cont, segment, '\n')) lines.push_back(segment);
	ofstream ofs((c->sphenoinput).c_str());
	bool inpblock =false;
	string blockname;
	bool nextline=false;
    for(auto it = lines.begin(); it !=lines.end(); it++) {
		if(inpblock && regex_search(*it, regex("^\\s*Block\\s+"))) inpblock=false; //We reached the next block
		if(!inpblock){
			for(auto itt : c->inputblocks) {
				if(regex_search(*it, regex("^\\s*Block\\s+"+ itt + "\\s+#"))) {
					inpblock=true;
					blockname=itt;
					}
				}
			if(!inpblock){
				ofs << *it << endl;
				continue; //next line because we didnt find a new inputblock
				}
			}
		if(blockname=="SPhenoInput") {
			if(regex_search(*it, regex("^\\s*13\\s+1\\s+#"))) it->assign(" 13 0               # 3-Body decays: none (0), fermion (1), scalar (2), both (3) ");
			else if(regex_search(*it, regex("^\\s*16\\s+1\\s+#"))) it->assign(" 16 0              # One-loop decays ");
			else if(c->loop && regex_search(*it, regex("^\\s*55\\s+0\\s+#"))) it->assign(" 55 1               # Calculate loop corrected masses");
			else if(!(c->loop) && regex_search(*it, regex("^\\s*55\\s+1\\s+#"))) it->assign(" 55 0               # Calculate loop corrected masses");
			else if(regex_search(*it, regex("^\\s*75\\s+1\\s+#"))) it->assign(" 75 0               # Write WHIZARD files ");
			else if(regex_search(*it, regex("^\\s*76\\s+1\\s+#"))) it->assign(" 76 0               # Write HiggsBounds file ");
			else if(regex_search(*it, regex("^\\s*79\\s+1\\s+#"))) it->assign(" 79 0               # Write WCXF files (exchange format for Wilson coefficients) ");
			else if(regex_search(*it, regex("^\\s*7\\s+0\\s+#"))) it->assign("  7  1              # Skip 2-loop Higgs corrections ");
			ofs << *it << endl;
			continue; //Next line
			}
		for (auto itp = (c->inputparameters).begin(); itp!=(c->inputparameters).end();itp++) {
			if(blockname==itp->blockname && (!itp->block.empty())){
				string pattern("^\\s*");
				for(auto itb: itp->block) pattern.append(to_string(itb)+"\\s+");
				pattern.append("(-?[0-9]+.[0-9]+(?:E[+-][0-9]+)?)\\s+#");
				if(regex_search(*it, regex(pattern))) {
					for(auto itw=(c->werte).begin(); itw!=(c->werte).end();itw++) {
						if((itp->name == itw->first)) {
							ostringstream wertsci;
							wertsci << scientific << setprecision(7) << uppercase << itw->second; //Scientific notation, Input file always has precision 7
							it->assign(regex_replace(*it, regex("\\s(-?[0-9]+.[0-9]+(?:E[+-][0-9]+)?)\\s+#"), " "+wertsci.str()+"  #"));
							nextline=true;
							break; //We dont need to check the other parameters now
							}
						}
					}
				if(nextline){
					nextline=false;
					break; //We found the parameter so we dont need to check the other ones
					}
				}
			}
		ofs << *it << endl;
		}
	ofs.close();
}


void iterateparameters(Configuration *c, vector<InputParameter>::iterator iter)//Recursive iteration to go for all the parametervariations for every parameter
{
    if(iter != --((c->inputparameters).end()))
    {
		if(iter->term == "") { //If the parameter has either added values xor ranged with stepsize
			for(auto pv = (iter->values).begin(); pv != (iter->values).end(); pv++)
			{
				if(iter->values.empty()){
					stringstream error;
					error << "Input parameter " << iter->name <<" has no values!";
					throw error.str();
				}
				(c->werte).push_back(make_pair(iter->name,*pv));
				iter++; //Here we jump to the next parameter
				iterateparameters(c, iter);
				iter--; //Here we return to the current parameter for the next for loop
				for(auto itw=(c->werte).begin(); itw!=(c->werte).end(); itw++)//We need to clear the current parameter from c->werte
				{
					if(itw->first == iter->name) {
						(c->werte).erase(itw);
						break;
					}
				}
			}
		}
		else //A parameter with calculated values
		{
			c->werte.push_back(make_pair(iter->name, iter->calculatevalue(c->werte)));
			if(iter->values.empty()){
				stringstream error;
				error << "Input parameter " << iter->name <<" has no values after calculations!";
				throw error.str();
			}
			(c->werte).push_back(make_pair(iter->name,iter->values.front()));
			iter++;
			iterateparameters(c, iter);
			iter--;
			for(auto itw=(c->werte).begin(); itw!=(c->werte).end(); itw++) {
				if(itw->first == iter->name) c->werte.erase(itw);
				break;
			}
		}
    }
	else //Last Run
	{
		if(iter->term!=""){ //A parameter with calculated values
			iter->calculatevalue(c->werte);
		}
		if(iter->values.empty()){
			stringstream error;
			error << "Input parameter " << iter->name <<" has no values!";
			throw error.str();
		}
		for(auto pv = (iter->values).begin(); pv != (iter->values).end(); pv++)
		{
			c->werte.push_back(make_pair(iter->name,*pv));
			if(Checkconditions(c)) //Check Inputparameterconditions
			{
				ChangeInputFile(c);
				RunSPheno(c);
			}
			else {
				ofstream ofs;
				ofs.open (c->results + "ErroneousInputs", ofstream::out | ofstream::app);
				string stringbuilder("");
				for(auto it = (c->werte).begin(); it != (c->werte).end(); it++)
				{
				ostringstream wert;
				wert <<fixed <<setprecision(c->precision) << scientific << it->second;
				stringbuilder += c->delimiter + it->first + "=" + wert.str();
				}
				boost::replace_all(stringbuilder, "_", " ");
				boost::replace_all(stringbuilder, "=", " = ");
				ofs << "InputConditions not met with input: " << stringbuilder << endl;
				ofs.close();
			}
			//We need to clean the werte vector from all added variables (calculated and LHA) except the inputparameters and constants
			for(auto itw=(c->werte).begin(); itw!=(c->werte).end(); itw++) {
				bool nomatch=true;
				for(auto itp=(c->inputparameters).begin(); itp!=(c->inputparameters).end(); itp++)
					if(itw->first == itp->name && itp->name != iter->name) nomatch=false;
				for(auto itc=(c->constants).begin(); itc!=(c->constants).end(); itc++)
					if(itw->first == itc->first) nomatch=false;
				if(nomatch){
					(c->werte).erase(itw); 
					itw--; //Erasing gives the next element but the for-loop also increments so we need to go back
				}
			}
		}
	}
}






void Writeresults(Configuration *c, string configfilename)
{ 

	string tabs = "\t";
	string fieldvariables = parseXML( parseXML( parseXML( parseXML(	configfilename, "VevaciousPlusPlusMainInput.InitializationFile",false),"VevaciousPlusPlusObjectInitialization.PotentialFunctionInitializationFile",false), "VevaciousPlusPlusPotentialFunctionInitialization.PotentialFunctionClass.ConstructorArguments.ModelFile", false), "VevaciousModelFile.FieldVariables", false);
	string path = c->vppoutput;
	auto filenames = Filenames(path);
	bool headerstable=false; //Assume we dont have a header yet.
	bool headermeta=false;
	int count(0);
	for(auto it : filenames) {	
		//try {
			if(it.find(".vout")!=string::npos) {
				vector<string> tempparams;
				string tagxml = parseXML(path+it, "parameter");
				boost::split(tempparams,tagxml,boost::is_any_of("\t"));
				vector<pair<string,string>> parameters;
				for(auto it : tempparams) parameters.push_back(make_pair(it.substr(0,it.find("=")) ,it.substr(it.find("=")+1) ));
				tempparams.clear();
				if((parseXML(path+it, "VevaciousResults.StableOrMetastable")).find("metastable")!=string::npos){ //METASTABLE.
					ofstream Results(c->results+"Results_metastable", ofstream::out | ofstream::app);
					if(!headermeta) {
						for(auto it : parameters) Results << it.first << tabs;
						Results <<"DSBRelDepth"<< tabs << "DSBFv";
						Results << fieldvariables << tabs;
						Results <<"PanicRelDepth"<<tabs;
						Results << "PanicFv";
						Results << fieldvariables << tabs;
						Results << "T=0_P(Survival)" << tabs;
						Results << "T=0_DSBLifetime" <<tabs;
						Results << "T!=0_P(Survival)"<<endl<<endl<<endl;
						headermeta=true;
					}
					for(auto it : parameters) Results << it.second << tabs;
					Results << setprecision(c->precision) << stod(parseXML(path+it, "VevaciousResults.DsbVacuum.RelativeDepth")) << tabs;
					Results << parseXML(path+it, "VevaciousResults.DsbVacuum.FieldValues") << tabs;
					Results << setprecision(c->precision) << stod(parseXML(path+it, "VevaciousResults.PanicVacuum.RelativeDepth")) << tabs;
					Results << parseXML(path+it, "VevaciousResults.PanicVacuum.FieldValues") << tabs
					<< parseXML(path+it, "VevaciousResults.ZeroTemperatureDsbSurvival.DsbSurvivalProbability")<<tabs
					<< parseXML(path+it, "VevaciousResults.ZeroTemperatureDsbSurvival.DsbLifetime")<<tabs
					<< parseXML(path+it, "VevaciousResults.NonZeroTemperatureDsbSurvival.DsbSurvivalProbability",true)<<endl;
				}
				else {
					ofstream Results(c->results+"Results_stable", ofstream::out | ofstream::app);	 //STABLE.
					if(!headerstable) {
						for(auto it : parameters) Results << it.first << tabs;
						Results << "RelDepth"<< tabs <<
						fieldvariables <<endl <<endl << endl;
						headerstable=true;
					}
					for(auto it : parameters) Results << it.second << tabs;
					Results << setprecision(c->precision) << stod(parseXML(path+it, "VevaciousResults.DsbVacuum.RelativeDepth")) << tabs; 
					Results << parseXML(path+it, "VevaciousResults.DsbVacuum.FieldValues")<<endl;
				}
			}
			count++;
	}
	cout << "Parsed "<< count << " Vpp outputs out of "<< filenames.size() << " files to " << c->results <<endl;
}


void ReadConfiguration(string file, Configuration *c, bool single=true){
	//Strings
	c->configfilename = file;
	c->spheno = parseXML(file,"SPheno.executable");
	c->sphenoinput = parseXML(file,"SPheno.inputfile");
	if(!single) { //If we use multiple runpoint input
		c->sphenooutput = parseXML(file,"VevaciousPlusPlusMainInput.ParameterPointSet.InputFolder");
		c->vppoutput = parseXML(file,"VevaciousPlusPlusMainInput.ParameterPointSet.OutputFolder");
	}
	else { //If we run every file seperately, Currently the only option without bugs in V++
		string ssphenooutput =  parseXML(file,"VevaciousPlusPlusMainInput.SingleParameterPoint.RunPointInput");
		string svppoutput =  parseXML(file,"VevaciousPlusPlusMainInput.SingleParameterPoint.OutputFilename");
		c->sphenooutput = ssphenooutput.substr(0,ssphenooutput.find_last_of("/")+1);	//Now substr to the last / since we only want to extract the directory
		c->vppoutput = svppoutput.substr(0,svppoutput.find_last_of("/")+1);
	}
	c->results = parseXML(file, "programspecific.Results");
	c->delimiter = parseXML(file, "programspecific.delimiter");
	if(c->delimiter == "."){
		c->delimiter='_';
		cout << "Delimiter can't be \".\", it has been changed to _" << endl;
	}
	c->VppExec = parseXML(file, "programspecific.VevaciousPlusPlusExec");
	c->loop = boost::lexical_cast<bool>(parseXML(file, "programspecific.Loop"));
	string prec = parseXML(file, "programspecific.precision",true); //optional
	c->precision = (prec=="")?7:stoi(prec); // 7 is standard value, precision of LesHouches.in standard
	c->options = parseXML(file, "programspecific.options", true);
	//For Parameters
	using boost::property_tree::ptree;
    ptree pt;
    read_xml(file, pt);
	BOOST_FOREACH( ptree::value_type const& v, pt.get_child("programspecific") ) {
        if( v.first == "inputparameter" ) {
			vector<string> values;
			vector<int> blocks;
            string name =v.second.get<string>("name");
			boost::optional<string> inpblockname = v.second.get_optional<string>("blockname");
			string sinpblockname = boost::get_optional_value_or(inpblockname,"MINPAR");
			boost::optional<string> optblock = v.second.get_optional<string>("block");
			string sblock = boost::get_optional_value_or(optblock,"-1");
			boost::optional<string> optbeginval = v.second.get_optional<string>("beginval");
			string sbeginval = boost::get_optional_value_or(optbeginval,"");
			boost::optional<string> optendval = v.second.get_optional<string>("endval");
			string sendval = boost::get_optional_value_or(optendval,sbeginval);
			boost::optional<string> paramstepsize = v.second.get_optional<string>("stepsize");
			string spsize = boost::get_optional_value_or(paramstepsize,"-1");
			boost::optional<string> optvals = v.second.get_optional<string>("values");
			string svals = boost::get_optional_value_or(optvals,"");
			bool inpcheck = false;
			for(auto it : c->inputblocks) if(it == sinpblockname) inpcheck=true;
			if(!inpcheck) c->inputblocks.push_back(sinpblockname);
			if(sblock != "-1") {
				vector<string> sblocks;
				boost::replace_all(sblock, "\t", " ");
				boost::replace_all(sblock, "\n", " ");
				boost::replace_all(sblock, ",", " ");
				boost::trim(sblock);
				sblock = regex_replace(sblock, regex("[\\s]{2,}"), " "); //Only one whitespace between values
				boost::split(sblocks, sblock, boost::is_any_of(" "));
				for(auto itb : sblocks) blocks.push_back(stoi(itb));
			}
			if(svals != "") { //All the user input values
				boost::replace_all(svals, "\t", " ");
				boost::replace_all(svals, "\n", " ");
				boost::replace_all(svals, ",", " ");
				boost::trim(svals);
				svals = regex_replace(svals, regex("[\\s]{2,}"), " "); //Only one whitespace between values
				boost::split(values, svals, boost::is_any_of(" "));
				}
			clearwhitespaces(&name);
			clearwhitespaces(&sinpblockname);
			clearwhitespaces(&spsize);
			clearwhitespaces(&sbeginval);
			clearwhitespaces(&sendval);
			int block = stoi(sblock);
			int pstepsize = stoi(spsize);

			stringstream error;
			if(pstepsize == -1 && sbeginval != sendval && values.empty()) {
				error << "There's no stepsize for " << name << " and the beginval is not the same as the endval" << endl;
				throw error.str();
			}
			if(pstepsize >0 && sbeginval == sendval) {
				error << "Parameter: "<< name << " has a non vanishing stepsize but the start and endvalue are the same, or you didn't include an endval" << endl;
				throw error.str();
			}
			if(sbeginval != ""){
				double beginval =stod(sbeginval);
				double endval = stod(sendval);
				auto ipp =  InputParameter(name, blocks, stoi(spsize), beginval, endval,sinpblockname);
				if(svals != "") for(auto itv : values) ipp.addvalue(stod(itv));
				(c->inputparameters).push_back(ipp);
			}
			else {
				if(values.empty()) {
					error << "The inputparameter " << name << " doesn't have startval and endval and also no values." << endl;
					throw error.str();
				}
				auto ipp = InputParameter(name, blocks, sinpblockname);
				for(auto itv : values) ipp.addvalue(stod(itv));
				(c->inputparameters).push_back(ipp);
			}
        }
		if(v.first=="inputparametercalc")
		{
			vector<int> blocks;
			string name = v.second.get<string>("name");
			string sblock= v.second.get<string>("block");
			string term = v.second.get<string>("term");
			boost::optional<string> inpblockname = v.second.get_optional<string>("blockname");
			string sinpblockname = boost::get_optional_value_or(inpblockname,"MINPAR");
			if(sblock != "") {
				vector<string> sblocks;
				boost::replace_all(sblock, "\t", " ");
				boost::replace_all(sblock, "\n", " ");
				boost::replace_all(sblock, ",", " ");
				boost::trim(sblock);
				sblock = regex_replace(sblock, regex("[\\s]{2,}"), " "); //Only one whitespace between values
				boost::split(sblocks, sblock, boost::is_any_of(" "));
				for(auto itb : sblocks) blocks.push_back(stoi(itb));
			}
			clearwhitespaces(&term);
			clearwhitespaces(&name);
			clearwhitespaces(&sinpblockname);
			(c->inputparameters).push_back(InputParameter(name, blocks, term,sinpblockname));
			bool inpcheck = false;
			for(auto it : c->inputblocks) if(it == sinpblockname) inpcheck=true;
			if(!inpcheck) c->inputblocks.push_back(sinpblockname);
		}
		if(v.first == "lhablock") {
			vector<int> blocks;
			string name = v.second.get<string>("name");
			string blockname = v.second.get<string>("blockname");
			string sblock= v.second.get<string>("block");
			if(sblock != "") {
				vector<string> sblocks;
				boost::replace_all(sblock, "\t", " ");
				boost::replace_all(sblock, "\n", " ");
				boost::replace_all(sblock, ",", " ");
				boost::trim(sblock);
				sblock = regex_replace(sblock, regex("[\\s]{2,}"), " "); //Only one whitespace between values
				boost::split(sblocks, sblock, boost::is_any_of(" "));
				for(auto itb : sblocks) blocks.push_back(stoi(itb));
			}
			clearwhitespaces(&name);
			clearwhitespaces(&blockname);
			(c->lhaparameters).push_back(LHAParameter(name,blockname,blocks));
		}
		if(v.first == "condition")
		{
			boost::optional<string> parcond = v.second.get_optional<string>("inputcondition");
			string pcond = boost::get_optional_value_or(parcond,"");
			boost::optional<string> calccond = v.second.get_optional<string>("lhacondition");
			string ccond = boost::get_optional_value_or(calccond,"");
			if(ccond == "" && pcond == ""){
				string error ("Either inputconditions or lhaconditions have to be given in the xml file. Or don't put a <condition> xmlbody");
				throw error;
			}
			if(ccond != "" && pcond != ""){
				string error("Only one condition per condition tag is allowed");
				throw error;
			}
			else if(pcond != "") {
				clearwhitespaces(&pcond);
				(c->paramconditions).push_back(pcond);
			}
			else if(ccond != "") {
				clearwhitespaces(&ccond);
				(c->lhaconditions).push_back(ccond);
			}
		}
		if(v.first=="lhacalc")
		{
			string name = v.second.get<string>("name");
			string term = v.second.get<string>("term");
			clearwhitespaces(&term);
			clearwhitespaces(&name);
			(c->lhaparameters).push_back(LHAParameter(name,term));
		}

		if(v.first=="constant")
		{
			string name = v.second.get<string>("name");
			string value = v.second.get<string>("value");
			clearwhitespaces(&value);
			clearwhitespaces(&name);
			double dvalue = stod(value);
			(c->constants).push_back(make_pair(name,dvalue));
			(c->werte).push_back(make_pair(name,dvalue)); //Registering the constants
		}
    }
	c->inputblocks.push_back("SPhenoInput");
}
#endif /* RUNNER_HPP_ */

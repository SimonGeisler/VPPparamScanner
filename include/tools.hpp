#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <random>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>
#include <cstdlib>
#include <regex>

#include <boost/foreach.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

using namespace std;

double randnumber(double min, double max)
{
	random_device rd;
	default_random_engine rn(rd());
	uniform_real_distribution<> dist;
	dist = uniform_real_distribution<>(min,max);
	return dist(rn);
}

void clearwhitespaces(string *inp) { 
	boost::trim(*inp);
	inp->assign(regex_replace(regex_replace(*inp, regex("\\n"), "\t") , regex("[\\s]{2,}"),"\t"));
}
string parseXML (string file, string xmlbody,bool optional=false){
	using boost::property_tree::ptree;
	ifstream is(file);
	if(!(is.is_open())){
		string error("ParseXML couldn't open the XML file ");
		error += file;
		throw error;
	}
	ptree pt;
	read_xml(is,pt);
	is.close();
	string str;
	if(optional) {
		boost::optional<string> v = pt.get_optional<string>(xmlbody);
		str = boost::get_optional_value_or(v,"");
	}
	else str = pt.get<string>(xmlbody);
	clearwhitespaces(&str);
	stringstream error;
	error << "Could not parse " << file <<" with the XMLBody " << xmlbody<< endl;
	if(str.empty() && !optional) throw error.str();
	return str;
}

void editXML(string file, string xmlbody, string xmlreplace)
{
	using boost::property_tree::ptree;
	string parent = xmlbody.substr(0,xmlbody.find_last_of('.'));
	string child = xmlbody.substr(xmlbody.find_last_of('.')+1);
	ptree tree;
	read_xml(file, tree,boost::property_tree::xml_parser::trim_whitespace);
	ptree &childtree = tree.get_child(parent);
	childtree.put(child,xmlreplace);
	boost::property_tree::xml_writer_settings<string> settings('\t', 1);
	boost::property_tree::write_xml(file, tree, locale(), boost::property_tree::xml_writer_make_settings< string >(' ', 4));
}


vector<string> Filenames(string path){ 
    DIR*    dir;
    dirent* pdir;
    vector<string> files;
    dir = opendir(path.c_str());
    while (pdir = readdir(dir)) {
        if(string(pdir->d_name) != "." && string(pdir->d_name) != "..") files.push_back(pdir->d_name);
    }
    return files;
}

void clearoutputfolder(string dirname, bool keep=true){
	auto files = Filenames(dirname);
	string filename;
	if(keep)
	{
		string renamefile;
		DIR* dir = opendir( (dirname+"old/").c_str() );
		if (ENOENT == errno) //Error : No such file or directory.
		{
			int err = system(("mkdir -p " + dirname+"old").c_str());
			if(err==-1){
				string error("Error creating the /old/ folder for Vppoutputs");
				throw error;
			}
		}
		
		if (dir)
		{
			for(auto it:files)
			{
				filename.assign(dirname+it);
				renamefile.assign(dirname+"old/"+it);
				ifstream iff(renamefile);
				if(iff.good()) remove( renamefile.c_str() ); //Remove Files in the old folder.
				rename( filename.c_str() , renamefile.c_str() ); //Move Files to old folder
			}
			closedir(dir);
		}
	}
	else
	{
		for(auto it:files)
		{
			filename.assign(dirname+ it);
			int err = remove( filename.c_str() ); //remove files
			if(err!=0){
				string error("Error removing files, check your permissions. File: ");
				error += filename;
				throw error;
			}
		}
	}
}
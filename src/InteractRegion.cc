// Implements the Region class
#include "InteractRegion.h"
#include "behavior_functions.h"

#include <iostream>
#include <string>


namespace mbmore {

// Globally scoped list of columns for the database
  /*
  std::set<std::string> InteractRegion::column_names = {
    "Enrich", "Auth", "Conflict", "Mil_Sp", "Reactors", "Mil_Iso", 
    "Sci_Net", "U_Reserve"};

  std::vector<std::string> InteractRegion::master_factors [] = {
    "Enrich", "Auth", "Conflict", "Mil_Sp", "Reactors", "Mil_Iso", 
    "Sci_Net", "U_Reserve"};
*/
  std::vector<std::string> InteractRegion::column_names;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InteractRegion::InteractRegion(cyclus::Context* ctx)
  : cyclus::Region(ctx) {
    //  kind_ = "InteractRegion";
  cyclus::Warn<cyclus::EXPERIMENTAL_WARNING>("the InteractRegion agent is experimental.");

}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  InteractRegion::~InteractRegion() {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void InteractRegion::Build(cyclus::Agent* parent) {
    cyclus::Agent::Build(parent);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::map<std::string, double>
  InteractRegion::GetWeights(std::string eqn_type) {
    //TODO: use eqn_type ot expand in offering PE or AQ results
    return p_wts;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void InteractRegion::EnterNotify() {
  cyclus::Region::EnterNotify();

  // Define column names for the database only once.
    std::string master_factors [] = {"Enrich", "Auth", "Conflict", "Mil_Sp", "Reactors", "Mil_Iso","Sci_Net", "U_Reserve"};
   if (column_names.size() == 0){
     std::map<std::string, double >::iterator f_it;
     //	   std::pair<std::string, std::vector<double> > >::iterator eqn_it;
     //  for(eqn_it = P_f.begin(); eqn_it != P_f.end(); eqn_it++) {
     for(f_it = p_wts.begin(); f_it != p_wts.end(); f_it++) {
       column_names.push_back(f_it->first);
     }
   }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Returns a map of regularly used factors and bool to indicate whether they are
// defined in this sim.
std::map<std::string, bool>
  InteractRegion::GetFactors(std::string eqn_type) {

  std::map<std::string, bool> present;
  std::map<std::string,double>::iterator factor_it;

  /*
  std::string master_factors [] = {				   
    "Enrich", "Auth", "Conflict", "Mil_Sp", "Reactors", "Mil_Iso", 
    "Sci_Net", "U_Reserve"};
  */

  int n_factors = sizeof(column_names) / sizeof(column_names[0]);

  for(int i = 0; i < n_factors; i++) {
    std::cout << "Master Factors: " << column_names[i] << std::endl;
    factor_it = p_wts.find(column_names[i]);
    if (factor_it == p_wts.end()) {   // factor isn't defined in input file
      present[column_names[i]]= false;
      //      std::cout << "FALSE for" << master_factors[i] << std::endl;
    }
    else {
      present[column_names[i]]= true;
    }
  }
  
  return present;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Determine the likelihood value for the equation at the current time,
// (where the current value of the equation is normalized to be between 0-1)
double InteractRegion::GetLikely(std::string phase, double eqn_val) {

  std::pair<std::string, std::vector<double> > likely_pair = likely_rescale[phase];
  std::string function = likely_pair.first;
  std::vector<double> constants = likely_pair.second;
  
  /*
  std::map<std::string, double> curr_likely;
  
  std::map<std::string,
	   std::pair<std::string, std::vector<double> > >::iterator eqn_it;

  for(eqn_it = likely_rescale.begin(); eqn_it != likely_rescale.end(); eqn_it++) {
    std::string phase = eqn_it->first;
    std::string function = eqn_it->second.first;
    std::vector<double> constants = eqn_it->second.second;
  */

    double phase_likely = CalcYVal(function, constants, eqn_val);
    std::cout << "phase " << phase << " fn " << function << "likely " << phase_likely << std::endl;
    return phase_likely;

    //    curr_likely[phase] = phase_likely;
    //  }
    //  return curr_likely;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Determine the Conflict or Military Isolation actor for the state at each
// timestep. Begin with the map of relations to all other states, sum these
// values and normalize. Then convert result to a 0-10 scale (0 == alliance,
// 5 == neutral, 10 == conflict)
double InteractRegion::GetInteractFactor(std::string eqn_type, std::string factor, std::string prototype) {
    
  std::map<std::string, std::map<std::string, int> > relations_map ;
  if ((eqn_type == "Pursuit") && (factor == "Conflict")){
    relations_map = p_conflict_map;
  }
  std::map<std::string, int> relations = relations_map[prototype];
  std::map<std::string, int>::iterator map_it;
  int net_relation = 0;
  for(map_it = relations.begin(); map_it != relations.end(); map_it++) {
    net_relation += map_it->second;
  }

  double fractional_val = static_cast<double>(net_relation)/relations.size();
  double scaled_val;
  if (fractional_val <= 0.0) {
    scaled_val = fractional_val*(-5.0) + 5.0;
  }
  else {
    scaled_val = fractional_val*5.0;
  }
  std::cout << "scaled conflict is " << scaled_val << std::endl;
  return scaled_val;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string InteractRegion::str() {
  std::string s = cyclus::Agent::str();

  s += " has insts: ";
  for (std::set<Agent*>::const_iterator inst = children().begin();
       inst != children().end();
       inst++) {
    s += (*inst)->prototype() + ", ";
  }
  return s;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Agent* ConstructInteractRegion(cyclus::Context* ctx) {
  return new InteractRegion(ctx);
}
  
}  // namespace mbmore

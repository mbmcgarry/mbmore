// Implements the Region class
#include "InteractRegion.h"
#include "behavior_functions.h"

#include <iostream>
#include <string>


namespace mbmore {

// Globally scoped list of columns for the database
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

  // Define Master List of column names for the database only once.
  std::string master_factors [] = { "Auth", "Conflict", "Enrich",
				    "Mil_Iso","Mil_Sp","Reactors",
				    "Sci_Net", "U_Reserve"};
  int n_factors = sizeof(master_factors) / sizeof(master_factors[0]);
  
  if (column_names.size() == 0){
    for(int f_it = 0; f_it < n_factors; f_it++) {
      std::cout << "EnterNotify adding " << master_factors[f_it] << std::endl;
      column_names.push_back(master_factors[f_it]);
    }
  }

  // Determine which factors are used in the simulation based on the defined
  // weights.
  p_present = DefinedFactors("Pursuit");
  //  a_present = DefinedFactors("Acquire");

  // Check weights to make sure they add to one, otherwise normalize
  double tot_weight = 0.0;
  std::map <std::string, double>::iterator wt_it;
  for(wt_it = p_wts.begin(); wt_it != p_wts.end(); wt_it++) {
    std::cout << "Adding " << wt_it->second << std::endl;
    tot_weight+= wt_it->second;
  }
  if (tot_weight == 0){
    cyclus::Warn<cyclus::VALUE_WARNING>("Weights must be defined!");
  }
  else if (tot_weight != 1.0) {
    for(wt_it = p_wts.begin(); wt_it != p_wts.end(); wt_it++) {
      wt_it->second = wt_it->second/tot_weight;
    }
  }

  //If conflict is defined, record initial conflict relations in database
  if (p_present["Conflict"] == true){
    std::string eqn_type = "Pursuit";
    for (auto const &ent1 : p_conflict_map) {
      for (auto const &ent2 : ent1.second){
	RecordConflictFactor(eqn_type, ent1.first, ent2.first, ent2.second);
      }
    }
  }
}


  /*
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InteractRegion::Tick() {
  // If any state has proliferated since the last time step, then adjust
  // factors of other states in response

  using cyclus::Agent;
  // TODO: Check all children of the simulation.
  //Are any prototypes "SecretSink"? Then ++ conflict with all other states

  int np = 0;
  Agent* me = this;
  std::set<cyclus::Agent*>::iterator cit;
  for (cit = cyclus::Agent::children().begin();
       cit != cyclus::Agent::children().end();
       ++cit) {
    Agent* a = *cit;
    if (a->prototype() == "SecretSink"){
      np++;
    }

}
  */

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Determines which factors are defined for this sim
std::map<std::string, bool>
  InteractRegion::DefinedFactors(std::string eqn_type) {

  std::map<std::string, double> wts;
  if (eqn_type == "Pursuit"){
    wts = p_wts;
  }

  std::map<std::string, bool> present;
  std::map<std::string,double>::iterator factor_it;
  int n_factors = column_names.size();
  std::cout << "number of master factors is " << n_factors << std::endl;
  //TODO: loop to get a_present also
  for(int i = 0; i < n_factors; i++) {
    factor_it = wts.find(column_names[i]);
    if (factor_it == wts.end()) {   // factor isn't defined in input file
      present[column_names[i]]= false;
    }
    else {
      present[column_names[i]]= true;
    }
  }
  return present;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Returns a map of regularly used factors and bool to indicate whether they are
// defined in this sim.
std::map<std::string, bool>
  InteractRegion::GetDefinedFactors(std::string eqn_type) {
  if (eqn_type == "Pursuit"){
    return p_present;
  }
  else {
    return a_present;
  }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Returns a map of regularly used factors and bool to indicate whether they are
// defined in this sim.
std::vector<std::string>& InteractRegion::GetMasterFactors() {
  return column_names;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Determine the likelihood value for the equation at the current time,
// (where the current value of the equation is normalized to be between 0-1)
double InteractRegion::GetLikely(std::string phase, double eqn_val) {

  std::pair<std::string, std::vector<double> > likely_pair =
    likely_rescale[phase];
  std::string function = likely_pair.first;
  std::vector<double> constants = likely_pair.second;
  
  double phase_likely = CalcYVal(function, constants, eqn_val);
  //  std::cout << "phase " << phase << " fn " << function << "likely " << phase_likely << std::endl;
  return phase_likely;
  
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Determine the Conflict or Military Isolation actor for the state at each
// timestep. Begin with the map of relations to all other states, sum these
// values and normalize. Then convert result to a 0-10 scale (0 == alliance,
// 5 == neutral, 10 == conflict)
double InteractRegion::GetInteractFactor(std::string eqn_type,
					 std::string factor,
					 std::string prototype) {
    
  std::map<std::string, std::map<std::string, double> > relations_map ;
  if ((eqn_type == "Pursuit") && (factor == "Conflict")){
    relations_map = p_conflict_map;
  }
  std::map<std::string, double> relations = relations_map[prototype];
  std::map<std::string, double>::iterator map_it;
  double net_relation = 0;
  for(map_it = relations.begin(); map_it != relations.end(); map_it++) {
    net_relation += map_it->second;
  }

  double fractional_val = static_cast<double>(net_relation)/relations.size();
  double scaled_val;

  // Convert from [-2,2] to [10,0]
  scaled_val = 2.5*(2.0 - fractional_val);

  /*
  if (fractional_val <= 0.0) {
    scaled_val = fractional_val*(-5.0) + 5.0;
  }
  else {
    scaled_val = 5.0 - fractional_val*5.0;
  }
  */

  //  std::cout << "raw conflict: " << fractional_val << "  scaled conflict:" << scaled_val << std::endl;
  return scaled_val;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Change the Conflict value for a state. If the simulation is symmetric,
// then the change in conflict value is mutual between the two states. Otherwise
// only the state whose change was initiated is affected, such that the two
// states may have different perspectives on their relationship.
void InteractRegion::ChangeConflictFactor(std::string eqn_type,
					  std::string this_state,
					  std::string other_state,
					  double new_val){
  if (eqn_type == "Pursuit"){
    p_conflict_map[this_state][other_state] = new_val;
    RecordConflictFactor(eqn_type, this_state, other_state, new_val);
    if (symmetric == 1){
      p_conflict_map[other_state][this_state] = new_val;
      RecordConflictFactor(eqn_type, other_state, this_state, new_val);
    }
  }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Record conflict factors for each pair at start of simulation and
// whenever they are changed 
void InteractRegion::RecordConflictFactor(std::string eqn_type,
					  std::string this_state,
					  std::string other_state,
					  double new_val){
  using cyclus::Context;
  using cyclus::Recorder;

  cyclus::Datum *d = context()->NewDatum("InteractRelations");
  d->AddVal("Time", context()->time());
  d->AddVal("PrimaryAgent", this_state);
  d->AddVal("SecondaryAgent", other_state);
  d->AddVal("Conflict", new_val);
  d->Record();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Change the Conflict value for a state. If the simulation is symmetric,
// then the change in conflict value is mutual between the two states. Otherwise
// only the state whose change was initiated is affected, such that the two
// states may have different perspectives on their relationship.

  void InteractRegion::GetNewProlifConflict(std::string prolif_state){
  std::string eqn_type = "Pursuit";
  for (auto const &state_it : p_conflict_map[prolif_state]){
    std::string affected_state = state_it.first;
    double cur_val = state_it.second;
    double new_val;
    if (cur_val > 0){
      new_val = 2;
    } else if (cur_val == 0){
      new_val = -1;
    } else {
      new_val = -2;
    }
    // If symmetric is off, then change will effect the other state's
    // perception of relationship. (If symmetric On then both are affected)
    ChangeConflictFactor("Pursuit", affected_state, prolif_state, new_val);
  }
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

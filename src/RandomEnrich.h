#ifndef MBMORE_SRC_ENRICHMENT_H_
#define MBMORE_SRC_ENRICHMENT_H_

#include <string>

#include "cyclus.h"
#include "sim_init.h"

namespace mbmore {

/// @class SWUConverter
///
/// @brief The SWUConverter is a simple Converter class for material to
/// determine the amount of SWU required for their proposed enrichment
class SWUConverter : public cyclus::Converter<cyclus::Material> {
 public:
  SWUConverter(double feed_commod, double tails) : feed_(feed_commod),
    tails_(tails) {}
  virtual ~SWUConverter() {}

  /// @brief provides a conversion for the SWU required
  virtual double convert(
      cyclus::Material::Ptr m,
      cyclus::Arc const * a = NULL,
      cyclus::ExchangeTranslationContext<cyclus::Material>
          const * ctx = NULL) const {
    cyclus::toolkit::Assays assays(feed_, cyclus::toolkit::UraniumAssay(m),
                                   tails_);
    return cyclus::toolkit::SwuRequired(m->quantity(), assays);
  }

  /// @returns true if Converter is a SWUConverter and feed and tails equal
  virtual bool operator==(Converter& other) const {
    SWUConverter* cast = dynamic_cast<SWUConverter*>(&other);
    return cast != NULL &&
    feed_ == cast->feed_ &&
    tails_ == cast->tails_;
  }

 private:
  double feed_, tails_;
};

/// @class NatUConverter
///
/// @brief The NatUConverter is a simple Converter class for material to
/// determine the amount of natural uranium required for their proposed
/// enrichment
class NatUConverter : public cyclus::Converter<cyclus::Material> {
 public:
  NatUConverter(double feed_commod, double tails) : feed_(feed_commod),
    tails_(tails) {}
  virtual ~NatUConverter() {}

  /// @brief provides a conversion for the amount of natural Uranium required
  virtual double convert(
      cyclus::Material::Ptr m,
      cyclus::Arc const * a = NULL,
      cyclus::ExchangeTranslationContext<cyclus::Material>
          const * ctx = NULL) const {
    cyclus::toolkit::Assays assays(feed_, cyclus::toolkit::UraniumAssay(m),
                                   tails_);
    cyclus::toolkit::MatQuery mq(m);
    std::set<cyclus::Nuc> nucs;
    nucs.insert(922350000);
    nucs.insert(922380000);

    double natu_frac = mq.mass_frac(nucs);
    double natu_req = cyclus::toolkit::FeedQty(m->quantity(), assays);
    return natu_req / natu_frac;
  }

  /// @returns true if Converter is a NatUConverter and feed and tails equal
  virtual bool operator==(Converter& other) const {
    NatUConverter* cast = dynamic_cast<NatUConverter*>(&other);
    return cast != NULL &&
    feed_ == cast->feed_ &&
    tails_ == cast->tails_;
  }

 private:
  double feed_, tails_;
};

///  The RandomEnrich is based on the Cycamore Enrich facility.
///  It is a simple Agent that enriches natural
///  uranium in a Cyclus simulation. It does not explicitly compute
///  the physical enrichment process, rather it calculates the SWU
///  required to convert an source uranium recipe (ie. natural uranium)
///  into a requested enriched recipe (ie. 4% enriched uranium), given
///  the natural uranium inventory constraint and its SWU capacity
///  constraint.
///
///  The RandomEnrich facility requests an input commodity and associated recipe
///  whose quantity is its remaining inventory capacity.  All facilities
///  trading the same input commodity (even with different recipes) will
///  offer materials for trade.  The RandomEnrich facility accepts any input
///  materials with enrichments less than its tails assay, as long as some
///  U235 is present, and preference increases with U235 content.  If no
///  U235 is present in the offered material, the trade preference is set
///  to -1 and the material is not accepted.  Any material components other
///  other than U235 and U238 are sent directly to the tails buffer.
///
///  The RandomEnrich facility can bid on requests for its output commodity
///  up to the maximum allowed enrichment (if not specified, default is 100%)
///  It bids on either the request quantity, or the maximum quanity allowed
///  by its SWU constraint or natural uranium inventory, whichever is lower.
///  If multiple output commodities with different enrichment levels are
///  requested and the facility does not have the SWU or quantity capacity
///  to meet all requests, the requests are fully, then partially filled
///  in unspecified but repeatable order.
///
///  The RandomEnrich facility offers its tails as an output commodity with
///  no associated recipe.  Bids for tails are constrained only by total
///  tails inventory.
///
///  The custom features of Random Enrich are:
///  variable tails assay, inspector swipe tests, and bidding behavior can be
///  set to occur at Every X timestep or at Random timesteps (see README for
//// full implementation documentation)
///
 
class RandomEnrich : public cyclus::Facility {
#pragma cyclus note {   	  \
  "niche": "enrichment facility",				  \
  "doc":								\
  "The RandomEnrich facility based on the Cycamore Enrich facility. " \
  "It is a simple agent that enriches natural "	 \
  "uranium in a Cyclus simulation. It does not explicitly compute "	\
  "the physical enrichment process, rather it calculates the SWU "	\
  "required to convert an source uranium recipe (i.e. natural uranium) " \
  "into a requested enriched recipe (i.e. 4% enriched uranium), given " \
  "the natural uranium inventory constraint and its SWU capacity " \
  "constraint."							\
  "\n\n"								\
  "The RandomEnrich facility requests an input commodity and associated " \
  "recipe whose quantity is its remaining inventory capacity.  All " \
  "facilities trading the same input commodity (even with different " \
  "recipes) will offer materials for trade.  The RandomEnrich facility " \
  "accepts any input materials with enrichments less than its tails assay, "\
  "as long as some U235 is present, and preference increases with U235 " \
  "content.  If no U235 is present in the offered material, the trade " \
  "preference is set to -1 and the material is not accepted.  Any material " \
  "components other than U235 and U238 are sent directly to the tails buffer."\
  "\n\n"								\
  "The RandomEnrich facility will bid on requests for its output commodity "\
  "up to the maximum allowed enrichment (if not specified, default is 100%) "\
  "It bids on either the request quantity, or the maximum quanity allowed " \
  "by its SWU constraint or natural uranium inventory, whichever is lower. " \
  "If multiple output commodities with different enrichment levels are " \
  "requested and the facility does not have the SWU or quantity capacity " \
  "to meet all requests, the requests are fully, then partially filled " \
  "in unspecified but repeatable order."				\
  "\n\n"								\
  "Accumulated tails inventory is offered for trading as a specifiable " \
  "output commodity."\
  "\n\n" \
  "The custom features of Random Enrich are: variable tails assay, inspector "\
  " swipe tests, and " \
  "bidding behavior can be set to occur at Every X timestep or at Random "\
  "timesteps (see README for full implementation documentation ",\
}
  
 public:
  // --- Module Members ---
  ///    Constructor for the RandomEnrich class
  ///    @param ctx the cyclus context for access to simulation-wide parameters
  RandomEnrich(cyclus::Context* ctx);

  ///     Destructor for the RandomEnrich class
  virtual ~RandomEnrich();

  #pragma cyclus

  ///     Print information about this agent
  virtual std::string str();
  // ---

  // --- Facility Members ---
  /// perform module-specific tasks when entering the simulation
  virtual void Build(cyclus::Agent* parent);
  // ---

  // --- Agent Members ---
  ///  Each facility is prompted to do its beginning-of-time-step
  ///  stuff at the tick of the timer.

  ///  @param time is the time to perform the tick
  virtual void Tick();

  ///  Each facility is prompted to its end-of-time-step
  ///  stuff on the tock of the timer.

  ///  @param time is the time to perform the tock
  virtual void Tock();

  /// @brief The RandomEnrich request Materials of its given
  /// commodity.
  virtual std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr>
      GetMatlRequests();

  /// @brief The RandomEnrich adjusts preferences for offers of
  /// natural uranium it has received to maximize U-235 content
  /// Any offers that have zero U-235 content are not accepted
  virtual void AdjustMatlPrefs(cyclus::PrefMap<cyclus::Material>::type& prefs);
 
  /// @brief The RandomEnrich place accepted trade Materials in their
  /// Inventory
  virtual void AcceptMatlTrades(
      const std::vector< std::pair<cyclus::Trade<cyclus::Material>,
      cyclus::Material::Ptr> >& responses);

  /// @brief Responds to each request for this facility's commodity.  If a given
  /// request is more than this facility's inventory or SWU capacity, it will
  /// offer its minimum of its capacities.
  virtual std::set<cyclus::BidPortfolio<cyclus::Material>::Ptr>
    GetMatlBids(cyclus::CommodMap<cyclus::Material>::type&
    commod_requests);

  /// @brief respond to each trade with a material enriched to the appropriate
  /// level given this facility's inventory
  ///
  /// @param trades all trades in which this trader is the supplier
  /// @param responses a container to populate with responses to each trade
  virtual void GetMatlTrades(
    const std::vector< cyclus::Trade<cyclus::Material> >& trades,
    std::vector<std::pair<cyclus::Trade<cyclus::Material>,
    cyclus::Material::Ptr> >& responses);
  // ---

  ///  @brief Determines if a particular material is a valid request to respond
  ///  to.  Valid requests must contain U235 and U238 and must have a relative
  ///  U235-to-U238 ratio less than this facility's tails_assay().
  ///  @return true if the above description is met by the material
  bool ValidReq(const cyclus::Material::Ptr mat);

  /// Determines whether EF is offering bids on a timestep
  bool trade_timestep;

  ///  @brief Determines if a particular request will be responded to
  ///  based on user specification such as maximum allowed enrichment
  ///  or other behavior parameters.
  virtual cyclus::BidPortfolio<cyclus::Material>::Ptr
    ConsiderMatlRequests(cyclus::CommodMap<cyclus::Material>::type&
		     commod_requests);

  inline double SwuCapacity() const { return swu_capacity; }

  inline const cyclus::toolkit::ResBuf<cyclus::Material>& Tails() const {
    return tails;
  } 
  // Tails assay at each timestep. Re-assessed at each Tick if sigma_tails > 0
  double curr_tails_assay ;

  // Total HEU (defined as > 20% enrichment) produced in the simulation at each
  // timestep
  double net_heu;

  // Presence of heu in the enrichment facility. Once it is present, it will
  // remain present for the rest of the simulation
  bool HEU_present;

  // Find the simulation duration
  //  cyclus::SimInfo si_;
  int simdur = context()->sim_info().duration;
  
 private:
  ///   @brief adds a material into the natural uranium inventory
  ///   @throws if the material is not the same composition as the feed_recipe
  void AddMat_(cyclus::Material::Ptr mat);

  ///   @brief generates a request for this facility given its current state.
  ///   Quantity of the material will be equal to remaining inventory size.
  cyclus::Material::Ptr Request_();

  ///  @brief Generates a material offer for a given request. The response
  ///  composition will be comprised only of U235 and U238 at their relative
  ///  ratio in the requested material. The response quantity will be the
  ///  same as the requested commodity.
  ///
  ///  @param req the requested material being responded to
  cyclus::Material::Ptr Offer_(cyclus::Material::Ptr req);

  cyclus::Material::Ptr Enrich_(cyclus::Material::Ptr mat, double qty);

  ///  @brief calculates the feed assay based on the unenriched inventory
  double FeedAssay();

  ///  @brief records and enrichment with the cyclus::Recorder
  void RecordRandomEnrich_(double natural_u, double swu);

  /// @brief if an inspection is performed, the resulting fraction of
  /// positive swipes/total swipes is recorded in the database for each
  /// unique sampling location
  void RecordInspection_();

  #pragma cyclus var { \
    "tooltip": "feed commodity",					\
    "doc": "feed commodity that the enrichment facility accepts",	\
    "uilabel": "Feed Commodity",                                    \
    "uitype": "incommodity" \
  }
  std::string feed_commod;
  
  #pragma cyclus var { \
    "tooltip": "feed recipe",						\
    "doc": "recipe for enrichment facility feed commodity",		\
    "uilabel": "Feed Recipe",                                   \
    "uitype": "recipe" \
  }
  std::string feed_recipe;
  
  #pragma cyclus var { \
    "tooltip": "product commodity",					\
    "doc": "product commodity that the enrichment facility generates",	 \
    "uilabel": "Product Commodity",                                     \
    "uitype": "outcommodity" \
  }
  std::string product_commod;
  
  #pragma cyclus var {							\
    "tooltip": "tails commodity",					\
    "doc": "tails commodity supplied by enrichment facility",		\
    "uilabel": "Tails Commodity",                                   \
    "uitype": "outcommodity" \
  }
  std::string tails_commod;

  #pragma cyclus var {							\
    "default": 0.003, "tooltip": "tails assay",				\
    "uilabel": "Tails Assay",                               \
    "doc": "tails assay from the enrichment process",       \
  }
  double tails_assay;

  #pragma cyclus var {"default": 0, "tooltip": "standard deviation of tails",\
                          "doc": "standard deviation (FWHM) of the normal " \
                                 "distribution used to generate tails " \
                                 "assay (if 0 then no distribution is " \
                                 "calculated and assay is constant in time." \
  }
  double sigma_tails;  

  #pragma cyclus var {							\
    "default": 0, "tooltip": "initial uranium reserves (kg)",		\
    "uilabel": "Initial Feed Inventory",				\
    "doc": "amount of natural uranium stored at the enrichment "	\
    "facility at the beginning of the simulation (kg)"			\
  }
  double initial_feed;

  #pragma cyclus var {							\
    "default": 1e299, "tooltip": "max inventory of feed material (kg)", \
    "uilabel": "Maximum Feed Inventory",                                \
    "doc": "maximum total inventory of natural uranium in "		\
           "the enrichment facility (kg)"     \
  }
  double max_feed_inventory;
 
  #pragma cyclus var { \
    "default": 1.0,						\
    "tooltip": "maximum allowed enrichment fraction",		\
    "doc": "maximum allowed weight fraction of U235 in product",\
    "uilabel": "Maximum Allowed RandomEnrich", \
    "schema": '<optional>'				     	   \
        '          <element name="max_enrich">'			   \
        '              <data type="double">'			   \
        '                  <param name="minInclusive">0</param>'   \
        '                  <param name="maxInclusive">1</param>'   \
        '              </data>'					   \
        '          </element>'					   \
        '      </optional>'					   \
  }
  double max_enrich;

  #pragma cyclus var { \
    "default": 1,		       \
    "userlevel": 10,							\
    "tooltip": "Rank Material Requests by U235 Content",		\
    "uilabel": "Prefer feed with higher U235 content", \
    "doc": "turn on preference ordering for input material "		\
           "so that EF chooses higher U235 content first" \
  }
  bool order_prefs;
  double initial_reserves;
  //***
  #pragma cyclus var {"default": "None", "tooltip": "social behavior" ,	\
                          "doc": "type of social behavior used in trade " \
                                 "decisions: None, Every, Random " \
                                 "where behav_interval describes the " \
                                 "time interval for behavior action"}
  std::string social_behav;

  #pragma cyclus var {"default": 0, "tooltip": "interval for behavior",\
                      "doc": "interval of social behavior: Every or "\
                             "EveryRandom.  If 0 then behavior is not " \
                             "implemented"}
  double behav_interval;

  #pragma cyclus var {"default": 0, "tooltip": "amount of HEU that accrues in"\
                              " cascade before being removed for shipment",\
                      "doc": "If non-zero and no social_behav, HEU cascade "\
                             "is emptied only when this qty of HEU has "\
                             "accumulated."}
  double heu_ship_qty;

  #pragma cyclus var {"default": 0, "tooltip": "interval for inspections",\
                      "doc": "average interval for inspections, will always "\
                             "be implemented with EveryRandom.  If 0" \
                             "then Inspections table is not filled out. If "\
                             "negative then RNG is queried but no inspections"\
                             "are recorded (to preserve reproducibility)."}
  int inspect_freq;
 
  #pragma cyclus var {"default": 10, "tooltip": "number of swipes per "	\
                             "inspection sample",      \
                      "doc": "number of swipes taken for a single sampled "\
                             "location by an inspector. Only used if " \
			     "inspection frequency is non-zero. Each swipe " \
			     "can have a false result as defined by false_pos "\
			     "and false_neg"}
  int n_swipes;

  #pragma cyclus var {"default": 0, "tooltip": "rate of false-positive swipes",\
                      "doc": "average rate of swipes that falsely detect HEU "\
                             "when in fact no HEU was present.  Applied "\
			     "individually to each swipe in a sample"}
  double false_pos;

  #pragma cyclus var {"default": 0, "tooltip": "rate of false-negative swipes",\
                      "doc": "average rate of swipes that falsely report no "\
                             "HEU when in fact HEU is present.  Applied "\
			     "individually to each swipe in a sample"}
  double false_neg;

  #pragma cyclus var {"default": 0, "tooltip": "Seed for RNG" ,		\
                          "doc": "seed on current system time if set to -1," \
                                 " otherwise seed on number defined"}
  int rng_seed;
  //***
  
  #pragma cyclus var {						       \
    "default": 1e299,						       \
    "tooltip": "SWU capacity (kgSWU/month)",			       \
    "uilabel": "SWU Capacity",                                         \
    "doc": "separative work unit (SWU) capacity of enrichment "		\
           "facility (kgSWU/timestep) "                                     \
  }
  double swu_capacity;

  double current_swu_capacity;

  #pragma cyclus var { 'capacity': 'max_feed_inventory' }
  cyclus::toolkit::ResBuf<cyclus::Material> inventory;  // natural u
  #pragma cyclus var {}
  cyclus::toolkit::ResBuf<cyclus::Material> tails;  // depleted u

  // used to total intra-timestep swu and natu usage for meeting requests -
  // these help enable time series generation.
  double intra_timestep_swu_;
  double intra_timestep_feed_;
  
  friend class RandomEnrichTest;
  // ---
};
 
}  // namespace mbmore

#endif // MBMORE_SRC_ENRICHMENT_FACILITY_H_

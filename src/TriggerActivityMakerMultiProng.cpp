/**
 * @file TriggerActivityMakerMultiProng.cpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "triggeralgs/MultiProng/TriggerActivityMakerMultiProng.hpp"
//#include "TRACE/trace.h"
//#define TRACE_NAME "TriggerActivityMakerMultiProng"
#include <vector>
#include <algorithm>

using namespace triggeralgs;

void
TriggerActivityMakerMultiProng::operator()(const TriggerPrimitive& input_tp,
                                               std::vector<TriggerActivity>& output_ta)
{
  // The first time operator() is called, reset the window object.
  if (m_current_window.is_empty()) {
    m_current_window.reset(input_tp);
    m_primitive_count++;
    return;
  }

  // If the difference between the current TP's start time and the start of the window
  // is less than the specified window size, add the TP to the window.
  if ((input_tp.time_start - m_current_window.time_start) < m_window_length) {
    m_current_window.add(input_tp);
  }

  // Check MultiProng Candidate ======================================================
  // We've filled the window, now require a sufficient length activity AND that it has
  // some potential 'multi-prongness' about it, and then issue a trigger activity.
  else if (longest_activity().size() > m_adjacency_threshold) {
     
     // We have a good length acitivity, now search for Bragg peak and kinks
     std::vector<TriggerPrimitive> hitsCluster = longest_activity();
  
     // ================ INSERT MULTI-PRONG CHECKS HERE!!! ===================
     // Hi Josh - you can perform your checks on hitsCluster which corresponds
     // to the largest piece of activity in the current visual field of the
     // activity maker. If your checks pass, make sure to then follow with the
     // below lines that emit a trigger activity up the chain to the TCMaker(s).

     std::cout << "Emitting a trigger for candidate MultiProng event." << std::endl;
     output_ta.push_back(construct_ta());
     m_current_window.reset(input_tp);

  }

  // Otherwise, slide the window along using the current TP.
  else {
    m_current_window.move(input_tp, m_window_length);
  }

  m_primitive_count++;
  return;
}

void
TriggerActivityMakerMultiProng::configure(const nlohmann::json& config)
{
  // FIX ME: Use some schema here. Also can't work out how to pass booleans.
  if (config.is_object()) {
    if (config.contains("trigger_on_n_channels"))
      m_trigger_on_n_channels = config["trigger_on_n_channels"];
    if (config.contains("n_channels_threshold"))
      m_n_channels_threshold = config["n_channels_threshold"];
    if (config.contains("window_length"))
      m_window_length = config["window_length"];
    if (config.contains("trigger_on_adjacency"))
      m_trigger_on_adjacency = config["trigger_on_adjacency"];
    if (config.contains("adj_tolerance"))
      m_adj_tolerance = config["adj_tolerance"];
    if (config.contains("adjacency_threshold"))
      m_adjacency_threshold = config["adjacency_threshold"];
  }

}

TriggerActivity
TriggerActivityMakerMultiProng::construct_ta() const
{

  TriggerPrimitive latest_tp_in_window = m_current_window.inputs.back();

  TriggerActivity ta;
  ta.time_start = m_current_window.time_start;
  ta.time_end = latest_tp_in_window.time_start + latest_tp_in_window.time_over_threshold;
  ta.time_peak = latest_tp_in_window.time_peak;
  ta.time_activity = latest_tp_in_window.time_peak;
  ta.channel_start = latest_tp_in_window.channel;
  ta.channel_end = latest_tp_in_window.channel;
  ta.channel_peak = latest_tp_in_window.channel;
  ta.adc_integral = m_current_window.adc_integral;
  ta.adc_peak = latest_tp_in_window.adc_peak;
  ta.detid = latest_tp_in_window.detid;
  ta.type = TriggerActivity::Type::kTPC;
  ta.algorithm = TriggerActivity::Algorithm::kMultiProng;
  ta.inputs = m_current_window.inputs;

  return ta;
}

std::vector<TriggerPrimitive>
TriggerActivityMakerMultiProng::longest_activity() const
{
  // This function attempts to return a vector of hits that correspond to the longest
  // piece of activity in the current window. The logic follows that from the HMA
  // check_adjacency() function and further details can be found there.
  std::vector<TriggerPrimitive> trackHits;
  std::vector<TriggerPrimitive> finalHits; // The vector of track hits, which we return

  uint16_t adj = 1;              // Initialise adjacency, 1 for the first wire.
  uint16_t max = 0;
  unsigned int channel = 0;      // Current channel ID
  unsigned int next_channel = 0; // Next channel ID
  unsigned int next = 0;         // The next position in the hit channels vector
  unsigned int tol_count = 0;    // Tolerance count, should not pass adj_tolerance

  // Generate a channelID ordered list of hit channels for this window
  std::vector<TriggerPrimitive> hitList;
  for (auto tp : m_current_window.inputs) {
    hitList.push_back(tp);
  }
  std::sort(hitList.begin(), hitList.end(), [](TriggerPrimitive a, TriggerPrimitive b)
            { return a.channel < b.channel; });

  // ADAJACENCY LOGIC ====================================================================
  // =====================================================================================
  // Adjcancency Tolerance = Number of times prepared to skip missed hits before resetting 
  // the adjacency count. This accounts for things like dead channels / missed TPs. The 
  // maximum gap is 4 which comes from tuning on December 2021 coldbox data, and June 2022 
  // coldbox runs.
  for (ulong i = 0; i < hitList.size(); ++i) {

    next = (i + 1) % hitList.size(); // Loops back when outside of channel list range
    channel = hitList.at(i).channel;
    next_channel = hitList.at(next).channel; // Next channel with a hit

    if (trackHits.size() == 0 ){ trackHits.push_back(hitList.at(i)); }

    // End of vector condition.
    if (next_channel == 0) { next_channel = channel - 1; }

    // Skip same channel hits for adjacency counting, but add to the track!
    if (next_channel == channel) { 
      trackHits.push_back(hitList.at(next));
      continue; }

    // If next hit is on next channel, increment the adjacency count.
    else if (next_channel == channel + 1){ 
      trackHits.push_back(hitList.at(next));
      ++adj; }

    // If next channel is not on the next hit, but the 'second next', increase adjacency 
    // but also tally up with the tolerance counter.
    else if (((next_channel == channel + 2) || (next_channel == channel + 3) ||
              (next_channel == channel + 4) || (next_channel == channel + 5))
             && (tol_count < m_adj_tolerance)) {
      trackHits.push_back(hitList.at(next));
      ++adj;
      for (uint i = 0 ; i < next_channel-channel ; ++i){ ++tol_count; }
    }

    // If next hit isn't within reach, end the adjacency count and check for a new max.
    // Reset variables for next iteration.
    else {
      if (adj > max) { 
        max = adj;
        finalHits.clear(); // Clear previous track
        for (auto h : trackHits){
          finalHits.push_back(h);
        }  
      }
      adj = 1;
      tol_count = 0;
      trackHits.clear();
    }
  }

  return finalHits;
}

// ===============================================================================================
// ===============================================================================================
// Functions below this line are for debugging purposes.
// ===============================================================================================

void
TriggerActivityMakerMultiProng::add_window_to_record(Window window)
{
  m_window_record.push_back(window);
  return;
}


// Function to dump the details of the TA window currently on record
void
TriggerActivityMakerMultiProng::dump_window_record()
{
  std::ofstream outfile;
  outfile.open("window_record_tam.csv", std::ios_base::app);

  for (auto window : m_window_record) {
    outfile << window.time_start << ",";
    outfile << window.inputs.back().time_start << ",";
    outfile << window.inputs.back().time_start - window.time_start << ","; // window_length - from TP start times
    outfile << window.adc_integral << ",";
    outfile << window.n_channels_hit() << ",";       // Number of unique channels with hits
    outfile << window.inputs.size() << ",";          // Number of TPs in Window
    outfile << window.inputs.back().channel << ",";  // Last TP Channel ID
    outfile << window.inputs.front().channel << ","; // First TP Channel ID
    outfile << longest_activity().size() << std::endl;             // New adjacency value for the window
  }

  outfile.close();

  m_window_record.clear();

  return;
}

// Function to add current TP details to a text file for testing and debugging.
void
TriggerActivityMakerMultiProng::dump_tp(TriggerPrimitive const& input_tp)
{
  std::ofstream outfile;
  outfile.open("coldbox_tps.txt", std::ios_base::app);

  // Output relevant TP information to file
  outfile << input_tp.time_start << " ";          // Start time of TP
  outfile << input_tp.time_over_threshold << " "; // in multiples of 25
  outfile << input_tp.time_peak << " ";           //
  outfile << input_tp.channel << " ";             // Offline channel ID
  outfile << input_tp.adc_integral << " ";        // ADC Sum
  outfile << input_tp.adc_peak << " ";            // ADC Peak Value
  outfile << input_tp.detid << " ";               // Det ID - Identifies detector element, APA or PDS part etc...
  outfile << input_tp.type << std::endl;          // This should now write out TPs in the same 'coldBox' way.
  outfile.close();

  return;
}


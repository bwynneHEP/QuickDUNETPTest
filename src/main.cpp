#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#include <chrono>
using namespace std::chrono;

#include "triggeralgs/TriggerPrimitive.hpp"
#include "triggeralgs/TriggerActivity.hpp"
#include "triggeralgs/MultiProng/TriggerActivityMakerMultiProng.hpp"
using namespace triggeralgs;

// Quick, standalone reader for DUNE TPs as text
int main( int argc, char * argv[] )
{
  // Command line argument check
  if ( argc != 2 )
  {
    std::cerr << "Please provide TP file as input" << std::endl;
    return 1;
  }

  // Load all TPs into a vector
  std::ifstream inputFile( argv[1] );
  std::string inputLine;
  int start_time, time_over, time_peak, channel, adc_sum, adc_peak, type, det_id, algo;
  std::vector< TriggerPrimitive > allPrimitives;
  while( std::getline( inputFile, inputLine ) )
  {
    // Parse a line in the input file
    int readValues = sscanf( inputLine.c_str(), "%d %d %d %d %d %d %d %d %d", &start_time, &time_over, &time_peak, &channel, &adc_sum, &adc_peak, &type, &det_id, &algo );
    if ( readValues != 9 )
    {
      std::cerr << "Input file format is not correct" << std::endl;
      return 2;
    }

    // Create a TP from the input data
    dunedaq::trgdataformats::TriggerPrimitive tp;
    tp.time_start = start_time;
    tp.time_over_threshold = time_over;
    tp.time_peak = time_peak;
    tp.channel = channel;
    tp.adc_integral = adc_sum;
    tp.adc_peak = adc_peak;
    tp.type = TriggerPrimitive::Type(type);
    tp.detid = det_id;
    tp.algorithm = TriggerPrimitive::Algorithm(algo);
    allPrimitives.push_back( tp );
  }

  // Process all TPs
  float totalTime = 0.0;
  float maxTime = 0.0;
  float minTime = 1E10;
  TriggerActivityMakerMultiProng algorithmInstance;
  for ( const auto& tp : allPrimitives )
  {
    std::vector< TriggerActivity > output;

    // Time the algorithm
    auto startTime = steady_clock::now();
    algorithmInstance( tp, output );
    auto endTime = steady_clock::now();

    float workTime = (float)duration_cast< nanoseconds >( endTime - startTime ).count();
    totalTime += workTime;
    if ( workTime > maxTime ) maxTime = workTime;
    if ( workTime < minTime ) minTime = workTime;
  }
  std::cout << allPrimitives.size() << " TPs processed in " << totalTime / 1E9 << "s: " << totalTime / allPrimitives.size() << "ns mean, " << minTime << "ns min, " << maxTime << "ns max" << std::endl;

  return 0;
}

#include "base/types.hh"
#include <cstdint>
#include <fstream>
#include <initializer_list>
#include <iomanip>
#include <ios>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace gem5 {

using std::cout;
using std::endl;
using std::map;
using std::string;
using std::to_string;
using std::unordered_map;

// maintained in constructor of ClockedObject
extern Tick BCClockPeriod;

// define couterData class to record counters,
// counter struct: {double value, Cycle lastUpdateCycle}
class counterData {
public:
  double value;
  Cycles lastUpdateCycle;

  counterData() : value(0), lastUpdateCycle(0) {}
};

class bcData {
public:
  uint64_t value;
  Cycles lastUpdateCycle;

  bcData() : value(0), lastUpdateCycle(0) {}
};

class burstCounter {

  typedef std::unordered_map<string, std::unordered_map<string, bcData>> BCMap;

public:
  // eventInXXCyclesV2: unordered_maps that record events that occur within
  // certain cycles, used for checkOrderV2 method.
  unordered_map<string, counterData> events;
  map<string, counterData> sortedEvents;
  BCMap eventIn16Cycles;
  BCMap eventIn32Cycles;
  BCMap eventIn64Cycles;
  BCMap eventIn128Cycles;
  BCMap eventIn256Cycles;
  BCMap eventIn16CyclesV2;
  BCMap eventIn32CyclesV2;
  BCMap eventIn64CyclesV2;
  BCMap eventIn128CyclesV2;
  BCMap eventIn256CyclesV2;

  void update(string name, double value, Tick curTick) {
    Cycles curCycle = Cycles((curTick + BCClockPeriod - 1) / BCClockPeriod);
    events[name].value = value;
    events[name].lastUpdateCycle = curCycle;
    checkOrder(name, curCycle);
    checkOrderV2(name, curCycle);
  }

  void printEventNames() {
    if (sortedEvents.empty()) {
      sortedEvents =
          std::map<string, counterData>(events.begin(), events.end());
    }
    cout << "recorded events: ";
    for (auto &iter : sortedEvents) {
      cout << iter.first << " ";
    }
    cout << endl << "all events count: " << events.size() << endl;
  }

  void printCounter(string name) { cout << events[name].value << endl; }

  void printAllCounters() {
    std::map<string, counterData> output_map(events.begin(), events.end());
    for (auto &iter : output_map) {
      cout << std::fixed << std::setprecision(0);
      cout << iter.first << ": " << iter.second.value << endl;
    }
  }

  double getValue(string name) { return events[name].value; }

  Cycles getLastUpdateCycle(string name) {
    return events[name].lastUpdateCycle;
  }

  void checkOrder(string name, Cycles curCycle) {
    for (auto &iter : events) {
      if (iter.first == name)
        continue;
      Cycles diff = curCycle - iter.second.lastUpdateCycle;
      if (diff == 0)
        return;
      else if (diff < 16) {
        eventIn16Cycles[iter.first][name].value++;
      } else if (diff < 32) {
        eventIn32Cycles[iter.first][name].value++;
      } else if (diff < 64) {
        eventIn64Cycles[iter.first][name].value++;
      } else if (diff < 128) {
        eventIn128Cycles[iter.first][name].value++;
      } else if (diff < 256) {
        eventIn256Cycles[iter.first][name].value++;
      }
    }
  }

  void checkOrderV2(string name, Cycles curCycle) {
    for (auto &iter : events) {
      if (iter.first == name)
        continue;
      // iter.first = events EARLY then current update
      // map[iter.first][name] = early -> now
      Cycles diff = curCycle - iter.second.lastUpdateCycle;
      if (diff == 0)
        return;
      std::vector<std::pair<int, BCMap *>> thresholdsAndEvents = {
          {16, &eventIn16CyclesV2},
          {32, &eventIn32CyclesV2},
          {64, &eventIn64CyclesV2},
          {128, &eventIn128CyclesV2},
          {256, &eventIn256CyclesV2}};

      for (const auto &[threshold, eventMap] : thresholdsAndEvents) {
        if (diff < threshold &&
            curCycle - (*eventMap)[iter.first][name].lastUpdateCycle >
                threshold) {
          (*eventMap)[iter.first][name].value++;
          break; // exit the loop once a match is found
        }
      }
    }
  }

  /**
   * @brief This function is a member of the BurstCounter class and is used
   * to dump the data to a file.
   *
   * The output format of the file is like:
   * [[1,2,3],
   *  [4,5,6],
   *  [7,8,9]]
   *
   * @tparam N: 1 for account repeatly, 2 for only once in time window.
   **/
  template <int N> void dumpBCtoFile(int cycle, BCMap unsorted_map) {
    std::map<string, std::map<string, uint64_t>> sorted_map;
    if (sortedEvents.empty()) {
      sortedEvents = // if not run 16 first, may have bug
          std::map<string, counterData>(events.begin(), events.end());
    }
    if (unsorted_map.size() != sortedEvents.size()) {
      // check all events in sortedEvents, if not in unsorted_map, insert 0
      for (auto &evnet : sortedEvents) {
        if (unsorted_map.find(evnet.first) == unsorted_map.end()) {
          unsorted_map[evnet.first] = {};
        }
      }
    }
    if (sorted_map.empty()) {
      for (auto &A : unsorted_map) {
        // map[A][B] = A then B, A include all events, B not
        for (auto &evnet : sortedEvents) {
          if (A.second.find(evnet.first) != A.second.end()) {
            sorted_map[A.first][evnet.first] = A.second[evnet.first].value;
          } else { // assign 0 if B not exist
            sorted_map[A.first][evnet.first] = 0;
            // cout << "insert 0: " << A.first << "->" << evnet.first <<
            // endl;
          }
        }
      }
    }
    std::ofstream myfile("bc" + to_string(cycle) + "v" + to_string(N) + ".txt");
    myfile << "[";
    for (auto iter = sorted_map.begin(); iter != sorted_map.end(); ++iter) {
      myfile << "[";
      bool first = true;
      for (auto it = iter->second.begin(); it != iter->second.end(); ++it) {
        myfile << (first ? "" : ",") << it->second;
        first = false;
      }
      myfile << "]" << (std::next(iter) != sorted_map.end() ? ",\n" : "");
    }
    myfile << "]";
    myfile.close();
  }
};

} // namespace gem5

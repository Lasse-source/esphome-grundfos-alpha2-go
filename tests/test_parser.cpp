// Host-side test of the frame parser used in packages/grundfos_alpha2_go.yaml.
// Feeds sample frames in 20-byte pieces (like small BLE notifications) and checks the decoded values.
// Build:  g++ -std=c++17 -o test_parser test_parser.cpp && ./test_parser sample_frames.txt
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <fstream>

struct S {
  const char *n; int cnt = 0; float last = NAN;
  void publish_state(float v) { cnt++; last = v; }
};
S grundfos_voltage{"voltage"}, grundfos_current{"current"}, grundfos_power{"power"}, grundfos_speed{"speed"},
    grundfos_flow{"flow m3/h"}, grundfos_head{"head m"}, grundfos_starts{"starts"}, grundfos_runtime{"runtime h"};
static int warnings = 0;
#define id(x) x
#define ESP_LOGW(tag, ...) do { warnings++; printf("LOGW[%s] ", tag); printf(__VA_ARGS__); printf("\n"); } while (0)
#define ESP_LOGD(tag, ...) do { } while (0)

float parse(const std::vector<uint8_t> &x) {
#include "parser_body.cpp"
}

static void feed(const std::vector<uint8_t> &fr, size_t piece) {
  for (size_t o = 0; o < fr.size(); o += piece) {
    std::vector<uint8_t> c(fr.begin() + o, fr.begin() + std::min(fr.size(), o + piece));
    parse(c);
  }
}

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } } while (0)

int main(int argc, char **argv) {
  std::ifstream in(argc > 1 ? argv[1] : "sample_frames.txt");
  std::string line;
  std::vector<std::vector<uint8_t>> frames;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::vector<uint8_t> fr;
    for (size_t i = 0; i + 1 < line.size(); i += 2) fr.push_back((uint8_t) std::stoi(line.substr(i, 2), nullptr, 16));
    frames.push_back(fr);
  }
  CHECK(frames.size() == 3, "expected 3 sample frames");

  // 1) normal operation, split into 20-byte notifications
  for (auto &f : frames) feed(f, 20);
  CHECK(grundfos_power.cnt == 1 && grundfos_power.last > 5 && grundfos_power.last < 200, "power in 5..200 W");
  CHECK(grundfos_voltage.cnt == 1 && grundfos_voltage.last > 200 && grundfos_voltage.last < 260, "voltage in 200..260 V");
  CHECK(grundfos_current.cnt == 1 && grundfos_current.last > 0.01 && grundfos_current.last < 1.0, "current in 0.01..1 A");
  CHECK(grundfos_speed.cnt == 1 && grundfos_speed.last > 500 && grundfos_speed.last < 6000, "speed in 500..6000 rpm");
  CHECK(grundfos_flow.cnt == 1 && grundfos_flow.last > 0.0 && grundfos_flow.last < 10, "flow in 0..10 m3/h");
  CHECK(grundfos_head.cnt == 1 && grundfos_head.last > 0.0 && grundfos_head.last < 10, "head in 0..10 m");
  CHECK(grundfos_starts.cnt == 1 && grundfos_starts.last >= 0, "starts >= 0");
  CHECK(grundfos_runtime.cnt == 1 && grundfos_runtime.last > 0, "runtime > 0");

  // 2) frames arriving as one big chunk, and byte by byte
  for (auto &f : frames) feed(f, 1000);
  for (auto &f : frames) feed(f, 1);
  CHECK(grundfos_power.cnt == 3, "power published for every chunking");

  // pick the power reply (type byte 0x30) for the next two tests
  std::vector<uint8_t> power_frame;
  for (auto &f : frames) if (f.size() > 13 && f[5] == 0x30) power_frame = f;
  CHECK(!power_frame.empty(), "sample file contains a power reply");

  // 3) a corrupted frame must be dropped (bad CRC) and not published
  int before = grundfos_power.cnt;
  auto bad = power_frame;
  bad[bad.size() - 1] ^= 0x55;
  feed(bad, 20);
  CHECK(grundfos_power.cnt == before, "frame with bad CRC is not published");
  CHECK(warnings >= 1, "bad CRC logs a warning");

  // 4) garbage before a valid frame: parser must resynchronise
  std::vector<uint8_t> junk = {0x00, 0x11, 0x22, 0x33, 0x44};
  for (auto b : power_frame) junk.push_back(b);
  feed(junk, 20);
  CHECK(grundfos_power.cnt == before + 1, "resync after garbage");

  printf("power=%.2f W voltage=%.1f V current=%.3f A speed=%.0f rpm flow=%.2f m3/h head=%.2f m starts=%.0f runtime=%.1f h\n",
         grundfos_power.last, grundfos_voltage.last, grundfos_current.last, grundfos_speed.last, grundfos_flow.last,
         grundfos_head.last, grundfos_starts.last, grundfos_runtime.last);
  printf(failures ? "%d test(s) FAILED\n" : "all tests passed\n", failures);
  return failures ? 1 : 0;
}

#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/input.h>
#include <unistd.h>

#define test_bit(bit, array)                                                   \
  ((array[(bit) / (8 * sizeof(long))] >> ((bit) % (8 * sizeof(long)))) & 1)

int main() {
  const char *device =
      "/dev/input/event17"; // kind of hardcoded controller’s event device
  int fd = open(device, O_RDWR);
  if (fd < 0) {
    perror("open");
    return 1;
  }

  // Query force feedback capabilities
  unsigned long features[20];
  memset(features, 0, sizeof(features));
  if (ioctl(fd, EVIOCGBIT(EV_FF, sizeof(features)), features) < 0) {
    perror("ioctl");
    close(fd);
    return 1;
  }

  for (auto i : features) {
    std::cout << i << ", ";
  }
  std::cout << std::endl;

  if (!test_bit(FF_RUMBLE, features)) {
    std::cerr << "Device does not support rumble\n";
    close(fd);
    return 1;
  }

  // Upload an effect
  struct ff_effect effect{};
  effect.type = FF_RUMBLE;
  effect.id = -1;                            // let the driver assign
  effect.u.rumble.strong_magnitude = 0x9999; // half strength
  effect.u.rumble.weak_magnitude = 0x9999;   // half strength

  if (ioctl(fd, EVIOCSFF, &effect) < 0) {
    perror("EVIOCSFF");
    close(fd);
    return 1;
  }

  // Play the effect
  struct input_event play{};
  play.type = EV_FF;
  play.code = effect.id;
  play.value = 1; // start
  if (write(fd, &play, sizeof(play)) < 0)
    perror("write play");

  usleep(5 * 1e6); // rumble for 2 seconds

  // Stop the effect
  play.value = 1;
  if (write(fd, &play, sizeof(play)) < 0)
    perror("write stop");

  close(fd);
  return 0;
}

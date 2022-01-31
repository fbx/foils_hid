#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>

#include <ela/ela.h>
#include <foils/hid.h>
#include <foils/hid_device.h>

static void
status(struct foils_hid *client, enum foils_hid_state state)
{
	const char *st = NULL;

	switch (state) {
	case FOILS_HID_IDLE:
		st = "idle";
		break;
	case FOILS_HID_CONNECTING:
		st = "connecting";
		break;
	case FOILS_HID_CONNECTED:
		st = "connected";
		break;
	case FOILS_HID_RESOLVE_FAILED:
		st = "resolve failed";
		break;
	case FOILS_HID_DROPPED:
		st = "dropped";
		break;
	}

	printf("Status: %s\n", st);
}

static void
feature_report(struct foils_hid *client,
	       uint32_t device_id, uint8_t report_id,
	       const void *data, size_t datalen)
{
}

static void
output_report(struct foils_hid *client,
	      uint32_t device_id, uint8_t report_id,
	      const void *data, size_t datalen)
{
}

static void
feature_report_sollicit(struct foils_hid *client,
			uint32_t device_id, uint8_t report_id)
{
}

static const struct foils_hid_handler handler =
{
	.status = status,
	.feature_report = feature_report,
	.output_report = output_report,
	.feature_report_sollicit = feature_report_sollicit,
};

static void
hidraw_cb(struct ela_event_source *source, int fd,
	  uint32_t mask, void *data)
{
	struct foils_hid *client = data;
	uint8_t report[512];
	ssize_t ret;

	while (1) {
		ret = read(fd, report, sizeof (report));
		if (ret <= 0) {
			if (errno != EAGAIN)
				fprintf(stderr, "error: failed reading from device\n");
			break;
		}
		foils_hid_input_report_send(client, 0, 0, 1, report, ret);
	}
}

static int
hidraw_get_descriptor(int fd, struct foils_hid_device_descriptor *descriptor)
{
	static struct hidraw_report_descriptor hidraw_desc;
	int size;

	if (ioctl(fd, HIDIOCGRDESCSIZE, &size)) {
		perror("ioctl/HIDIOCGRDESCSIZE");
		return 1;
	}

	hidraw_desc.size = size;
	if (ioctl(fd, HIDIOCGRDESC, &hidraw_desc)) {
		perror("ioctl/HIDIOCGRDESC");
		return 1;
	}

	memset(descriptor, 0, sizeof (*descriptor));
	descriptor->descriptor = hidraw_desc.value;
	descriptor->descriptor_size = hidraw_desc.size;
	descriptor->version = 0x0100;
	ioctl(fd, HIDIOCGRAWNAME(DEVICE_NAME_LEN), descriptor->name);

	return 0;
}

int main(int argc, char **argv)
{
	struct foils_hid client;
	struct foils_hid_device_descriptor descriptor;
	struct ela_el *el;
	struct ela_event_source *ev;
	char *devpath;
	char *host;
	int port;
	int dev_fd;
	int ret = 1;

	if (argc < 3) {
		fprintf(stderr, "usage: %s /dev/hidrawX IP [PORT]\n", argv[0]);
		return 1;
	}

	devpath = argv[1];
	host = argv[2];
	port = argc > 3 ? atoi(argv[3]) : 24322;

	dev_fd = open(devpath, O_RDWR | O_NONBLOCK);
	if (dev_fd < 0) {
		fprintf(stderr, "failed to open %s: %m\n", devpath);
		return 1;
	}

	if (hidraw_get_descriptor(dev_fd, &descriptor)) {
		fprintf(stderr, "failed to get hid descriptor, is %s an hidraw device ?\n",
			devpath);
		goto dev_deinit;
	}

	printf("Forwarding device `%s' to %s:%d\n",
	       descriptor.name, host, port);

	el = ela_create(NULL);

	ret = foils_hid_init(&client, el, &handler, &descriptor, 1);
	if (ret) {
		fprintf(stderr, "Error creating client: %s\n", strerror(ret));
		goto ela_deinit;
	}

	ret = foils_hid_client_connect_hostname(&client, host, port, 0);
	if (ret) {
		fprintf(stderr, "Error starting connection: %s\n", strerror(ret));
		goto ela_deinit;
	}

	foils_hid_device_enable(&client, 0);

	ela_source_alloc(el, hidraw_cb, &client, &ev);
	ela_set_fd(el, ev, dev_fd, ELA_EVENT_READABLE);
	ela_add(el, ev);
	ela_run(el);

	ret = 0;

	foils_hid_deinit(&client);
ela_deinit:
	ela_close(el);
dev_deinit:
	close(dev_fd);
	return ret;
}

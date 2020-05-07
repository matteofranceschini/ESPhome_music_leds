#include "WiFiUdp.h"
#include "esphome.h"

using namespace esphome;

typedef struct rgbledstruct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgbLed_t;

// BUFFER_LEN: Maximum number of packets to hold in the buffer. Don't change
// this.
static const uint16_t BUFFER_LEN = 1024;
// the packet buffer to decode the new message
char packetBuffer[BUFFER_LEN];
// variable that holds the lenght of the last packet received
uint16_t packetlength = 0;
// N_PIXELS: The number of the LEDS on the led strip, must be even.
static const uint16_t N_PIXELS = 301;
// The local led buffer
rgbLed_t buffer_leds[N_PIXELS];
// Variable that tells if new leds are ready to be changed
bool new_led_available = false;

//#define SINGLE_BYTE

enum PLAYMODE
{
    MODE_ARTNET,
    MODE_MUSIC
};

class MusicLeds
{
private:
    uint16_t pixel_number = 0;

public:
    MusicLeds();
    ~MusicLeds();
    void ShowFrame(PLAYMODE CurrentMode, light::AddressableLight *p_it);
};

MusicLeds::MusicLeds() {}

MusicLeds::~MusicLeds() {}

void MusicLeds::ShowFrame(PLAYMODE CurrentMode, light::AddressableLight *p_it)
{
    switch (CurrentMode)
    {
        case MODE_ARTNET:
            //            ESP_LOGD("custom", "MODE_ARTNET");
            break;
        case MODE_MUSIC:
            //           ESP_LOGD("custom", "MODE_MUSIC");
            if (new_led_available)
            {
                // packet is:
                // byte 1,2: pixel number
                // byte 3,4,5: r,g,b
#ifdef SINGLE_BYTE
#define INCREMENT 4
#else
#define INCREMENT 5
#endif
                for (uint16_t i = 0; i < packetlength; i += INCREMENT)
                {
                    packetBuffer[packetlength] = 0;

#ifdef SINGLE_BYTE
                    pixel_number = packetBuffer[i];
#else
                    pixel_number = ((packetBuffer[i] << 8) +
                                    packetBuffer[i + 1]);  // the pixel number
#endif
                    // Cannot write the pixel number if greater
                    if (pixel_number < p_it->size())
                    {
                        light::ESPColor c;
#ifdef SINGLE_BYTE
                        c.r = (uint8_t)packetBuffer[i + 1];
                        c.g = (uint8_t)packetBuffer[i + 2];
                        c.b = (uint8_t)packetBuffer[i + 3];
#else
                        c.r = (uint8_t)packetBuffer[i + 2];
                        c.g = (uint8_t)packetBuffer[i + 3];
                        c.b = (uint8_t)packetBuffer[i + 4];
                        c.w = 0;
#endif
                        /*per diminuire il numero di dati da inviare sulla rete
                         * "spalmo" il numero di led che mi arrivano sui tre
                         * punti fondamentali: gli angoli e il centro*/
                        // draw the effect on the first third
                        /*                       if (pixel_number > 12)
                                                   (*p_it)[pixel_number - 12] =
                           c;
                                               // draw the effect in the middle
                                               if (pixel_number > p_it->size() /
                           2)
                                                   (*p_it)[((p_it->size() / 2))
                           - pixel_number] = c;
                                               // draw the effect in the last
                           third
                                               (*p_it)[p_it->size() -
                           pixel_number] = c;
                                               */
                        (*p_it)[pixel_number] = c;
                    }
                }

                new_led_available = false;
            }
            break;
    }
}

class MusicLeds music_leds;

class MusicReceiver : public Component
{
private:
public:
    // The port number for the udp transmission
    unsigned int localPort = 7777;
    // the socket for the connection
    WiFiUDP music_udp;
    // Remembers if we are already connected
    bool already_initialized = false;

    /*Called only once*/
    void setup() override { already_initialized = false; }
    void loop() override
    {
        while (wifi_wificomponent->is_connected() != true)
        {
            already_initialized = false;
            ESP_LOGD("LED_UDP", "UDP WAITING...");
            delay(500);
            return;
        }
        if (!already_initialized)
        {
            already_initialized = true;
            music_udp.begin(localPort);
            ESP_LOGD("LED_UDP", "UDP STARTED!");
        }
        // Read data over socket
        int packetSize = music_udp.parsePacket();

        // If packets have been received, interpret the command
        if (packetSize)
        {
            if (new_led_available == false)
            {
                packetlength      = music_udp.read(packetBuffer, BUFFER_LEN);
                new_led_available = true;
            }
            else
            {
                // se ho già un pacchetto ricevuto ma non ancora letto, quello
                // nuovo lo flusho
                music_udp.flush();
            }
        }
    }
};


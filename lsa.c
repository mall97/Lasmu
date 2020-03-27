#include <stdio.h> //printf
#include <fcntl.h>
#include <sys/stat.h>
#include "client/include/uxr/client/client.h"
#include "client-build/temp_install/include/ucdr/microcdr.h"
#include "client/examples/Deployment/HelloWorld.h"

#define STREAM_HISTORY  8
#define BUFFER_SIZE     UXR_CONFIG_SERIAL_TRANSPORT_MTU * STREAM_HISTORY

int open_port(void)
{
  int fd; /* File descriptor for the port */

  fd = open("/dev/ttyACM0", O_RDWR | O_NOCTTY | O_NDELAY);   //change port 
  if (fd == -1)
  {
    perror("open_port: Unable to open /dev/ttyf1 - ");
  }
  else
    fcntl(fd, F_SETFL, 0);

  return (fd);
}


int main()
{   
    //transport
    uxrSerialTransport Transport;
    uxrSerialPlatform SerialPlatform;
    int fd=open_port();
    uint8_t local_addr0=1;
    if(!uxr_init_serial_transport(&Transport, &SerialPlatform, fd, 0, local_addr0))
    {
            printf("Error at create transport\n");
    }

    //session
    uxrSession session;
    uxr_init_session(&session, &Transport.comm, 0xABCDABCD);
    //uxr_set_topic_callback(&session, on_topic, NULL);
    if(!uxr_create_session(&session))
    {
        printf("Error at create session.\n");
        return 1;
    }

    uint8_t output_reliable_stream_buffer[BUFFER_SIZE];
    uxrStreamId reliable_out = uxr_create_output_reliable_stream(&session, output_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

    uint8_t input_reliable_stream_buffer[BUFFER_SIZE];
    uxr_create_input_reliable_stream(&session, input_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

    //setup a participant
    uxrObjectId participant_id = uxr_object_id(0x01, UXR_PARTICIPANT_ID);
    const char* participant_xml = "<dds>"
                                    "<participant>"
                                        "<rtps>"
                                            "<name>default_xrce_participant</name>"
                                        "</rtps>"
                                    "</participant>"
                                "</dds>";
    uint16_t participant_req = uxr_buffer_create_participant_ref(&session, reliable_out, participant_id, 0,participant_xml, UXR_REPLACE);

    //setup a Publisher
    uxrObjectId publisher_id = uxr_object_id(0x01, UXR_PUBLISHER_ID);
    const char* publisher_xml = "";
    uint16_t publisher_req = uxr_buffer_create_publisher_xml(&session, reliable_out, publisher_id, participant_id, publisher_xml, UXR_REPLACE);

    //data writer
    uxrObjectId datawriter_id = uxr_object_id(0x01, UXR_DATAWRITER_ID);
    const char* datawriter_xml = "<dds>"
                                    "<data_writer>"
                                        "<topic>"
                                            "<kind>NO_KEY</kind>"
                                            "<name>HelloWorldTopic</name>"
                                            "<dataType>HelloWorld</dataType>"
                                        "</topic>"
                                    "</data_writer>"
                                "</dds>";
    uint16_t datawriter_req = uxr_buffer_create_datawriter_xml(&session, reliable_out, datawriter_id, publisher_id, datawriter_xml, UXR_REPLACE);

    //write data
    int i=0;
    uint32_t count = 0;

    while(1)
    {
        HelloWorld topic = {count++, "Hello DDS world!"};

        ucdrBuffer ub;
        uint32_t topic_size = HelloWorld_size_of_topic(&topic, 0);
        (void) uxr_prepare_output_stream(&session, reliable_out, datawriter_id, &ub, topic_size);
        (void) HelloWorld_serialize_topic(&ub, &topic);

        bool SendMsg=uxr_run_session_until_confirm_delivery(&session, 1000);

        i++;
    }
    //close client
    uxr_delete_session(&session);
    uxr_close_serial_transport(&Transport);
}
#include <ros/ros.h>
#include <tf2_msgs/TFMessage.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/CompressedImage.h>
#include <sensor_msgs/PointCloud2.h>
#include <geometry_msgs/Quaternion.h>
#include <geometry_msgs/Vector3.h>
#include <geometry_msgs/PoseStamped.h>
#include <visualization_msgs/MarkerArray.h>
#include <visualization_msgs/Marker.h>
#include <can_msgs/Frame.h>
#include <nmea_msgs/Sentence.h>
#include <pthread.h>

#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <csignal>

#include <librdkafka/rdkafkacpp.h>

using std::string;
using std::exception;
using std::unique_ptr;
using std::vector;
using std::cout;
using std::endl;
using std::cerr;

using RdKafka::Producer;
using RdKafka::Consumer;
using RdKafka::ConsumeCb;
using RdKafka::Conf;
using RdKafka::Topic;
using RdKafka::Message;
using RdKafka::ErrorCode;
using RdKafka::ERR__TIMED_OUT;
using RdKafka::ERR_NO_ERROR;
using RdKafka::ERR__PARTITION_EOF;
using RdKafka::ERR__UNKNOWN_TOPIC;
using RdKafka::ERR__UNKNOWN_PARTITION;

bool exit_eof = false;

static string brokers;
static string max_msg_size;
static string max_copy_msg_size;
static string recv_max_msg_size;
static string queue_max_msg_size;
static string fetch_max_msg_size;
static string queue_buffer_max_msg_size;
static string socket_send_buffer_size;
static string socket_recv_buffer_size;

static int32_t partition = Topic::PARTITION_UA;
static int64_t start_offset = Topic::OFFSET_END;
static string group_id;
static string auto_commit;

static string vehicle_id;
static string errstr;

static string tf_str;
static string pcd_str;
static string pose_str;
static string imu_str;
static string compressed_img_str;
static string img_str;
static string marker_array_str;
static string marker_str;
static string can_str;
static string nmea_str;

static vector<bool> tf_runnings;;
static vector<bool> pcd_runnings;
static vector<bool> pose_runnings;
static vector<bool> imu_runnings;
static vector<bool> compressed_img_runnings;
static vector<bool> img_runnings;
static vector<bool> marker_array_runnings;
static vector<bool> marker_runnings;
static vector<bool> can_runnings;
static vector<bool> nmea_runnings;


static vector<string> tf_topics;
static vector<string> pcd_topics;
static vector<string> pose_topics;
static vector<string> imu_topics;
static vector<string> compressed_img_topics;
static vector<string> img_topics;
static vector<string> marker_array_topics;
static vector<string> marker_topics;
static vector<string> can_topics;
static vector<string> nmea_topics;

static vector<ros::Publisher> tf_publishers;
static vector<ros::Publisher> pcd_publishers;
static vector<ros::Publisher> pose_publishers;
static vector<ros::Publisher> imu_publishers;
static vector<ros::Publisher> compressed_img_publishers;
static vector<ros::Publisher> img_publishers;
static vector<ros::Publisher> marker_array_publishers;
static vector<ros::Publisher> marker_publishers;
static vector<ros::Publisher> can_publishers;
static vector<ros::Publisher> nmea_publishers;

static vector<unique_ptr<Topic>> kafka_tf_topics;
static vector<unique_ptr<Topic>> kafka_pcd_topics;
static vector<unique_ptr<Topic>> kafka_pose_topics;
static vector<unique_ptr<Topic>> kafka_imu_topics;
static vector<unique_ptr<Topic>> kafka_compressed_img_topics;
static vector<unique_ptr<Topic>> kafka_img_topics;
static vector<unique_ptr<Topic>> kafka_marker_array_topics;
static vector<unique_ptr<Topic>> kafka_marker_topics;
static vector<unique_ptr<Topic>> kafka_can_topics;
static vector<unique_ptr<Topic>> kafka_nmea_topics;

static vector<unique_ptr<Consumer>> tf_consumers;
static vector<unique_ptr<Consumer>> pcd_consumers;
static vector<unique_ptr<Consumer>> pose_consumers;
static vector<unique_ptr<Consumer>> imu_consumers;
static vector<unique_ptr<Consumer>> compressed_img_consumers;
static vector<unique_ptr<Consumer>> img_consumers;
static vector<unique_ptr<Consumer>> marker_array_consumers;
static vector<unique_ptr<Consumer>> marker_consumers;
static vector<unique_ptr<Consumer>> can_consumers;
static vector<unique_ptr<Consumer>> nmea_consumers;


static vector<pthread_t> tf_pthreads;
static vector<pthread_t> pcd_pthreads;
static vector<pthread_t> pose_pthreads;
static vector<pthread_t> imu_pthreads;
static vector<pthread_t> compressed_img_pthreads;
static vector<pthread_t> img_pthreads;
static vector<pthread_t> marker_array_pthreads;
static vector<pthread_t> marker_pthreads;
static vector<pthread_t> can_pthreads;
static vector<pthread_t> nmea_pthreads;


/*
 *  tf2_msgs::TFMessage
 *
 */
void tf_msg_consume(Message* message, int topic_index, void* opaque) {
  switch (message->err()) {
    case ERR__TIMED_OUT:      
      break;

    case ERR_NO_ERROR: 
      {
        // Real message 
        cout << "recv tf topic:" << topic_index << endl;
        cout << "Read tf msg at offset " << message->offset() << endl;
        if (message->key()) {
          cout << "Key: " << *message->key() << endl;
        }

        // Publish to ROS msg
        const unsigned char* blob = (const unsigned char*)message->payload();
        const size_t size = message->len ();        
        boost::shared_array<uint8_t> buffer(new uint8_t[size]);
        memcpy(buffer.get(), blob, size);
        ros::serialization::IStream stream(buffer.get(), size);
        cout << "recv tf size: " << size << endl;
        tf2_msgs::TFMessage data;
        ros::serialization::Serializer<tf2_msgs::TFMessage>::read(stream, data);                        
        tf_publishers[topic_index].publish(data);
        
        //cout << msg.get_payload() << endl;
        // Now commit the message
        //consumers[topic_index]->commitSync(message);        
      }
      break;

    case ERR__PARTITION_EOF:
      // Last message 
      if (exit_eof) {
        tf_runnings[topic_index] = false;
      }
      break;

    case ERR__UNKNOWN_TOPIC:
    case ERR__UNKNOWN_PARTITION:
      std::cerr << "tf Consume failed: " << message->errstr() << std::endl;
      tf_runnings[topic_index] = false;
      break;

    default:
      // Errors 
      std::cerr << "tf Consume failed: " << message->errstr() << std::endl;
      tf_runnings[topic_index] = false;
  }
}

void *tf_consumer_thread(void *arg) {
  int topic_index = *((int *)(&arg));
  while (ros::ok() && tf_runnings[topic_index]) {    
    // Try to consume a message
    Message *msg = tf_consumers[topic_index].get()->consume(kafka_tf_topics[topic_index].get(), partition, 1000);
    tf_msg_consume(msg, topic_index, NULL);
    delete msg;    
    
    tf_consumers[topic_index].get()->poll(0);
  }
  cout << "tf_thread: " << topic_index << " ends." << endl;
  return 0;
}

/*
 *  sensor_msgs::PointCloud2
 *
 */
void pcd_msg_consume(Message* message, int topic_index, void* opaque) {
  switch (message->err()) {
    case ERR__TIMED_OUT:      
      break;

    case ERR_NO_ERROR: 
      {
        // Real message 
        cout << "recv pcd topic:" << topic_index << endl;
        cout << "Read pcd msg at offset " << message->offset() << endl;
        if (message->key()) {
          cout << "Key: " << *message->key() << endl;
        }

        // Publish to ROS msg
        const unsigned char* blob = (const unsigned char*)message->payload();
        const size_t size = message->len ();        
        boost::shared_array<uint8_t> buffer(new uint8_t[size]);
        memcpy(buffer.get(), blob, size);
        ros::serialization::IStream stream(buffer.get(), size);
        cout << "recv pcd size: " << size << endl;
        sensor_msgs::PointCloud2 data;
        ros::serialization::Serializer<sensor_msgs::PointCloud2>::read(stream, data);                        
        pcd_publishers[topic_index].publish(data);
        
        //cout << msg.get_payload() << endl;
        // Now commit the message
        //consumers[topic_index]->commitSync(message);        
      }
      break;

    case ERR__PARTITION_EOF:
      // Last message 
      if (exit_eof) {
        pcd_runnings[topic_index] = false;
      }
      break;

    case ERR__UNKNOWN_TOPIC:
    case ERR__UNKNOWN_PARTITION:
      std::cerr << "pcd Consume failed: " << message->errstr() << std::endl;
      pcd_runnings[topic_index] = false;
      break;

    default:
      // Errors 
      std::cerr << "pcd Consume failed: " << message->errstr() << std::endl;
      pcd_runnings[topic_index] = false;
  }
}

void *pcd_consumer_thread(void *arg) {
  int topic_index = *((int *)(&arg));
  while (ros::ok() && pcd_runnings[topic_index]) {    
    // Try to consume a message
    Message *msg = pcd_consumers[topic_index].get()->consume(kafka_pcd_topics[topic_index].get(), partition, 1000);
    pcd_msg_consume(msg, topic_index, NULL);
    delete msg;    
    
    pcd_consumers[topic_index].get()->poll(0);
  }
  cout << "pcd_thread: " << topic_index << " ends." << endl;
  return 0;
}


/*
 *  geometry_msgs::PoseStamped
 *
 */
void pose_msg_consume(Message* message, int topic_index, void* opaque) {
  switch (message->err()) {
    case ERR__TIMED_OUT:      
      break;

    case ERR_NO_ERROR: 
      {
        // Real message 
        cout << "recv pose topic:" << topic_index << endl;
        cout << "Read pose msg at offset " << message->offset() << endl;
        if (message->key()) {
          cout << "Key: " << *message->key() << endl;
        }

        // Publish to ROS msg
        const unsigned char* blob = (const unsigned char*)message->payload();
        const size_t size = message->len ();        
        boost::shared_array<uint8_t> buffer(new uint8_t[size]);
        memcpy(buffer.get(), blob, size);
        ros::serialization::IStream stream(buffer.get(), size);
        cout << "recv pose size: " << size << endl;
        geometry_msgs::PoseStamped data;
        ros::serialization::Serializer<geometry_msgs::PoseStamped>::read(stream, data);                        
        pose_publishers[topic_index].publish(data);
        
        //cout << msg.get_payload() << endl;
        // Now commit the message
        //consumers[topic_index]->commitSync(message);        
      }
      break;

    case ERR__PARTITION_EOF:
      // Last message 
      if (exit_eof) {
        pose_runnings[topic_index] = false;
      }
      break;

    case ERR__UNKNOWN_TOPIC:
    case ERR__UNKNOWN_PARTITION:
      std::cerr << "pose Consume failed: " << message->errstr() << std::endl;
      pose_runnings[topic_index] = false;
      break;

    default:
      // Errors 
      std::cerr << "pose Consume failed: " << message->errstr() << std::endl;
      pose_runnings[topic_index] = false;
  }
}

void *pose_consumer_thread(void *arg) {
  int topic_index = *((int *)(&arg));
  while (ros::ok() && pose_runnings[topic_index]) {    
    // Try to consume a message
    Message *msg = pose_consumers[topic_index].get()->consume(kafka_pose_topics[topic_index].get(), partition, 1000);
    pose_msg_consume(msg, topic_index, NULL);
    delete msg;    
    
    pose_consumers[topic_index].get()->poll(0);
  }
  cout << "pose_thread: " << topic_index << " ends." << endl;
  return 0;
}

/*
 *  sensor_msgs::Imu
 *
 */
void imu_msg_consume(Message* message, int topic_index, void* opaque) {
  switch (message->err()) {
    case ERR__TIMED_OUT:      
      break;

    case ERR_NO_ERROR: 
      {
        // Real message 
        cout << "recv imu topic:" << topic_index << endl;
        cout << "Read imu msg at offset " << message->offset() << endl;
        if (message->key()) {
          cout << "Key: " << *message->key() << endl;
        }

        // Publish to ROS msg
        const unsigned char* blob = (const unsigned char*)message->payload();
        const size_t size = message->len ();        
        boost::shared_array<uint8_t> buffer(new uint8_t[size]);
        memcpy(buffer.get(), blob, size);
        ros::serialization::IStream stream(buffer.get(), size);
        cout << "recv imu size: " << size << endl;
        sensor_msgs::Imu data;
        ros::serialization::Serializer<sensor_msgs::Imu>::read(stream, data);                        
        imu_publishers[topic_index].publish(data);
        
        //cout << msg.get_payload() << endl;
        // Now commit the message
        //consumers[topic_index]->commitSync(message);        
      }
      break;

    case ERR__PARTITION_EOF:
      // Last message 
      if (exit_eof) {
        imu_runnings[topic_index] = false;
      }
      break;

    case ERR__UNKNOWN_TOPIC:
    case ERR__UNKNOWN_PARTITION:
      std::cerr << "imu Consume failed: " << message->errstr() << std::endl;
      imu_runnings[topic_index] = false;
      break;

    default:
      // Errors 
      std::cerr << "imu Consume failed: " << message->errstr() << std::endl;
      imu_runnings[topic_index] = false;
  }
}

void *imu_consumer_thread(void *arg) {
  int topic_index = *((int *)(&arg));
  while (ros::ok() && imu_runnings[topic_index]) {    
    // Try to consume a message
    Message *msg = imu_consumers[topic_index].get()->consume(kafka_imu_topics[topic_index].get(), partition, 1000);
    imu_msg_consume(msg, topic_index, NULL);
    delete msg;    
    
    imu_consumers[topic_index].get()->poll(0);
  }
  cout << "imu_thread: " << topic_index << " ends." << endl;
  return 0;
}

/*
 *  sensor_msgs::Image
 *
 */
void img_msg_consume(Message* message, int topic_index, void* opaque) {
  switch (message->err()) {
    case ERR__TIMED_OUT:      
      break;

    case ERR_NO_ERROR: 
      {
        // Real message 
        cout << "recv img topic:" << topic_index << endl;
        cout << "Read img msg at offset " << message->offset() << endl;
        if (message->key()) {
          cout << "Key: " << *message->key() << endl;
        }

        // Publish to ROS msg
        const unsigned char* blob = (const unsigned char*)message->payload();
        const size_t size = message->len ();        
        boost::shared_array<uint8_t> buffer(new uint8_t[size]);
        memcpy(buffer.get(), blob, size);
        ros::serialization::IStream stream(buffer.get(), size);
        cout << "recv img size: " << size << endl;
        sensor_msgs::Image data;
        ros::serialization::Serializer<sensor_msgs::Image>::read(stream, data);                        
        img_publishers[topic_index].publish(data);
        
        //cout << msg.get_payload() << endl;
        // Now commit the message
        //consumers[topic_index]->commitSync(message);        
      }
      break;

    case ERR__PARTITION_EOF:
      // Last message 
      if (exit_eof) {
        img_runnings[topic_index] = false;
      }
      break;

    case ERR__UNKNOWN_TOPIC:
    case ERR__UNKNOWN_PARTITION:
      std::cerr << "img Consume failed: " << message->errstr() << std::endl;
      img_runnings[topic_index] = false;
      break;

    default:
      // Errors 
      std::cerr << "img Consume failed: " << message->errstr() << std::endl;
      img_runnings[topic_index] = false;
  }
}

void *img_consumer_thread(void *arg) {
  int topic_index = *((int *)(&arg));
  while (ros::ok() && img_runnings[topic_index]) {    
    // Try to consume a message
    Message *msg = img_consumers[topic_index].get()->consume(kafka_img_topics[topic_index].get(), partition, 1000);
    imu_msg_consume(msg, topic_index, NULL);
    delete msg;    
    
    img_consumers[topic_index].get()->poll(0);
  }
  cout << "img_thread: " << topic_index << " ends." << endl;
  return 0;
}

/*
 *  sensor_msgs::CompressedImage
 *
 */
void compressed_img_msg_consume(Message* message, int topic_index, void* opaque) {
  switch (message->err()) {
    case ERR__TIMED_OUT:      
      break;

    case ERR_NO_ERROR: 
      {
        // Real message 
        cout << "recv compressed_img topic:" << topic_index << endl;
        cout << "Read compressed_img msg at offset " << message->offset() << endl;
        if (message->key()) {
          cout << "Key: " << *message->key() << endl;
        }

        // Publish to ROS msg
        const unsigned char* blob = (const unsigned char*)message->payload();
        const size_t size = message->len ();        
        boost::shared_array<uint8_t> buffer(new uint8_t[size]);
        memcpy(buffer.get(), blob, size);
        ros::serialization::IStream stream(buffer.get(), size);
        cout << "recv compressed_img size: " << size << endl;
        sensor_msgs::CompressedImage data;
        ros::serialization::Serializer<sensor_msgs::CompressedImage>::read(stream, data);                        
        compressed_img_publishers[topic_index].publish(data);
        
        //cout << msg.get_payload() << endl;
        // Now commit the message
        //consumers[topic_index]->commitSync(message);        
      }
      break;

    case ERR__PARTITION_EOF:
      // Last message 
      if (exit_eof) {
        compressed_img_runnings[topic_index] = false;
      }
      break;

    case ERR__UNKNOWN_TOPIC:
    case ERR__UNKNOWN_PARTITION:
      std::cerr << "compressed_img Consume failed: " << message->errstr() << std::endl;
      compressed_img_runnings[topic_index] = false;
      break;

    default:
      // Errors 
      std::cerr << "compressed_img Consume failed: " << message->errstr() << std::endl;
      compressed_img_runnings[topic_index] = false;
  }
}

void *compressed_img_consumer_thread(void *arg) {
  int topic_index = *((int *)(&arg));
  while (ros::ok() && compressed_img_runnings[topic_index]) {    
    // Try to consume a message
    Message *msg = compressed_img_consumers[topic_index].get()->consume(kafka_compressed_img_topics[topic_index].get(), partition, 1000);
    compressed_img_msg_consume(msg, topic_index, NULL);
    delete msg;    
    
    compressed_img_consumers[topic_index].get()->poll(0);
  }
  cout << "compressed_img_thread: " << topic_index << " ends." << endl;
  return 0;
}

/*
 *  visualization_msgs::MarkerArray
 *
 */
void marker_array_msg_consume(Message* message, int topic_index, void* opaque) {
  switch (message->err()) {
    case ERR__TIMED_OUT:      
      break;

    case ERR_NO_ERROR: 
      {
        // Real message 
        cout << "recv marker_array topic:" << topic_index << endl;
        cout << "Read marker_array msg at offset " << message->offset() << endl;
        if (message->key()) {
          cout << "Key: " << *message->key() << endl;
        }

        // Publish to ROS msg
        const unsigned char* blob = (const unsigned char*)message->payload();
        const size_t size = message->len ();        
        boost::shared_array<uint8_t> buffer(new uint8_t[size]);
        memcpy(buffer.get(), blob, size);
        ros::serialization::IStream stream(buffer.get(), size);
        cout << "recv marker_array size: " << size << endl;
        visualization_msgs::MarkerArray data;
        ros::serialization::Serializer<visualization_msgs::MarkerArray>::read(stream, data);                        
        marker_array_publishers[topic_index].publish(data);
        
        //cout << msg.get_payload() << endl;
        // Now commit the message
        //consumers[topic_index]->commitSync(message);        
      }
      break;

    case ERR__PARTITION_EOF:
      // Last message 
      if (exit_eof) {
        marker_array_runnings[topic_index] = false;
      }
      break;

    case ERR__UNKNOWN_TOPIC:
    case ERR__UNKNOWN_PARTITION:
      std::cerr << "marker_array Consume failed: " << message->errstr() << std::endl;
      marker_array_runnings[topic_index] = false;
      break;

    default:
      // Errors 
      std::cerr << "marker_array Consume failed: " << message->errstr() << std::endl;
      marker_array_runnings[topic_index] = false;
  }
}

void *marker_array_consumer_thread(void *arg) {
  int topic_index = *((int *)(&arg));
  while (ros::ok() && marker_array_runnings[topic_index]) {    
    // Try to consume a message
    Message *msg = marker_array_consumers[topic_index].get()->consume(kafka_marker_array_topics[topic_index].get(), partition, 1000);
    marker_array_msg_consume(msg, topic_index, NULL);
    delete msg;    
    
    marker_array_consumers[topic_index].get()->poll(0);
  }
  cout << "marker_array_thread: " << topic_index << " ends." << endl;
  return 0;
}

/*
 *  visualization_msgs::Marker
 *
 */
void marker_msg_consume(Message* message, int topic_index, void* opaque) {
  switch (message->err()) {
    case ERR__TIMED_OUT:      
      break;

    case ERR_NO_ERROR: 
      {
        // Real message 
        cout << "recv marker topic:" << topic_index << endl;
        cout << "Read marker msg at offset " << message->offset() << endl;
        if (message->key()) {
          cout << "Key: " << *message->key() << endl;
        }

        // Publish to ROS msg
        const unsigned char* blob = (const unsigned char*)message->payload();
        const size_t size = message->len ();        
        boost::shared_array<uint8_t> buffer(new uint8_t[size]);
        memcpy(buffer.get(), blob, size);
        ros::serialization::IStream stream(buffer.get(), size);
        cout << "recv marker size: " << size << endl;
        visualization_msgs::Marker data;
        ros::serialization::Serializer<visualization_msgs::Marker>::read(stream, data);                        
        marker_publishers[topic_index].publish(data);
        
        //cout << msg.get_payload() << endl;
        // Now commit the message
        //consumers[topic_index]->commitSync(message);        
      }
      break;

    case ERR__PARTITION_EOF:
      // Last message 
      if (exit_eof) {
        marker_runnings[topic_index] = false;
      }
      break;

    case ERR__UNKNOWN_TOPIC:
    case ERR__UNKNOWN_PARTITION:
      std::cerr << "marker Consume failed: " << message->errstr() << std::endl;
      marker_runnings[topic_index] = false;
      break;

    default:
      // Errors 
      std::cerr << "marker Consume failed: " << message->errstr() << std::endl;
      marker_runnings[topic_index] = false;
  }
}

void *marker_consumer_thread(void *arg) {
  int topic_index = *((int *)(&arg));
  while (ros::ok() && marker_runnings[topic_index]) {    
    // Try to consume a message
    Message *msg = marker_consumers[topic_index].get()->consume(kafka_marker_topics[topic_index].get(), partition, 1000);
    marker_msg_consume(msg, topic_index, NULL);
    delete msg;    
    
    marker_consumers[topic_index].get()->poll(0);
  }
  cout << "marker_thread: " << topic_index << " ends." << endl;
  return 0;
}

/*
 *  can_msgs::Frame
 *
 */
void can_msg_consume(Message* message, int topic_index, void* opaque) {
  switch (message->err()) {
    case ERR__TIMED_OUT:      
      break;

    case ERR_NO_ERROR: 
      {
        // Real message 
        cout << "recv can topic:" << topic_index << endl;
        cout << "Read can msg at offset " << message->offset() << endl;
        if (message->key()) {
          cout << "Key: " << *message->key() << endl;
        }

        // Publish to ROS msg
        const unsigned char* blob = (const unsigned char*)message->payload();
        const size_t size = message->len ();        
        boost::shared_array<uint8_t> buffer(new uint8_t[size]);
        memcpy(buffer.get(), blob, size);
        ros::serialization::IStream stream(buffer.get(), size);
        cout << "recv can size: " << size << endl;
        can_msgs::Frame data;
        ros::serialization::Serializer<can_msgs::Frame>::read(stream, data);                        
        can_publishers[topic_index].publish(data);
        
        //cout << msg.get_payload() << endl;
        // Now commit the message
        //consumers[topic_index]->commitSync(message);        
      }
      break;

    case ERR__PARTITION_EOF:
      // Last message 
      if (exit_eof) {
        can_runnings[topic_index] = false;
      }
      break;

    case ERR__UNKNOWN_TOPIC:
    case ERR__UNKNOWN_PARTITION:
      std::cerr << "can Consume failed: " << message->errstr() << std::endl;
      can_runnings[topic_index] = false;
      break;

    default:
      // Errors 
      std::cerr << "can Consume failed: " << message->errstr() << std::endl;
      can_runnings[topic_index] = false;
  }
}

void *can_consumer_thread(void *arg) {
  int topic_index = *((int *)(&arg));
  while (ros::ok() && can_runnings[topic_index]) {    
    // Try to consume a message
    Message *msg = can_consumers[topic_index].get()->consume(kafka_can_topics[topic_index].get(), partition, 1000);
    can_msg_consume(msg, topic_index, NULL);
    delete msg;    
    
    can_consumers[topic_index].get()->poll(0);
  }
  cout << "nmea_thread: " << topic_index << " ends." << endl;
  return 0;
}

/*
 *  nmea_msgs::Sentence
 *
 */
void nmea_msg_consume(Message* message, int topic_index, void* opaque) {
  switch (message->err()) {
    case ERR__TIMED_OUT:      
      break;

    case ERR_NO_ERROR: 
      {
        // Real message 
        cout << "recv nmea topic:" << topic_index << endl;
        cout << "Read nmea msg at offset " << message->offset() << endl;
        if (message->key()) {
          cout << "Key: " << *message->key() << endl;
        }

        // Publish to ROS msg
        const unsigned char* blob = (const unsigned char*)message->payload();
        const size_t size = message->len ();        
        boost::shared_array<uint8_t> buffer(new uint8_t[size]);
        memcpy(buffer.get(), blob, size);
        ros::serialization::IStream stream(buffer.get(), size);
        cout << "recv nmea size: " << size << endl;
        nmea_msgs::Sentence data;
        ros::serialization::Serializer<nmea_msgs::Sentence>::read(stream, data);                        
        nmea_publishers[topic_index].publish(data);
        
        //cout << msg.get_payload() << endl;
        // Now commit the message
        //consumers[topic_index]->commitSync(message);        
      }
      break;

    case ERR__PARTITION_EOF:
      // Last message 
      if (exit_eof) {
        nmea_runnings[topic_index] = false;
      }
      break;

    case ERR__UNKNOWN_TOPIC:
    case ERR__UNKNOWN_PARTITION:
      std::cerr << "nmea Consume failed: " << message->errstr() << std::endl;
      nmea_runnings[topic_index] = false;
      break;

    default:
      // Errors 
      std::cerr << "nmea Consume failed: " << message->errstr() << std::endl;
      nmea_runnings[topic_index] = false;
  }
}

void *nmea_consumer_thread(void *arg) {
  int topic_index = *((int *)(&arg));
  while (ros::ok() && nmea_runnings[topic_index]) {    
    // Try to consume a message
    Message *msg = nmea_consumers[topic_index].get()->consume(kafka_nmea_topics[topic_index].get(), partition, 1000);
    nmea_msg_consume(msg, topic_index, NULL);
    delete msg;    
    
    nmea_consumers[topic_index].get()->poll(0);
  }
  cout << "nmea_thread: " << topic_index << " ends." << endl;
  return 0;
}


void signalHandler( int signum ) {  
  // terminate program 
  for (size_t i=0; i < tf_runnings.size(); i++) 
    tf_runnings[i] = false;  
  for (size_t i=0; i < pcd_runnings.size(); i++) 
    pcd_runnings[i] = false;  
  for (size_t i=0; i < pose_runnings.size(); i++) 
    pose_runnings[i] = false;  
  for (size_t i=0; i < imu_runnings.size(); i++) 
    imu_runnings[i] = false;  
  for (size_t i=0; i < compressed_img_runnings.size(); i++) 
    compressed_img_runnings[i] = false;  
  for (size_t i=0; i < img_runnings.size(); i++) 
    img_runnings[i] = false;  
  for (size_t i=0; i < marker_array_runnings.size(); i++) 
    marker_array_runnings[i] = false;  
  for (size_t i=0; i < marker_runnings.size(); i++) 
    marker_runnings[i] = false;  
  for (size_t i=0; i < can_runnings.size(); i++) 
    can_runnings[i] = false;  
  for (size_t i=0; i < nmea_runnings.size(); i++) 
    nmea_runnings[i] = false;  
  // terminate program  
  ros::shutdown();  
}

int main(int argc, char* argv[]) {  
  ros::init(argc, argv, "kafka_consumer");
  
  ros::NodeHandle nh;
  ros::NodeHandle private_nh("~");
  
  // setting parameters
  int32_t partition_value;
  
  private_nh.param<string>("brokers", brokers, "localhost:9092");
  private_nh.param<string>("group_id", group_id, "group1");
  private_nh.param<int>("partition", partition_value, 0);
  private_nh.param<string>("vehicle_id", vehicle_id, "vehicle"); 
  private_nh.param<string>("tf_topics", tf_str, "/tf"); 
  private_nh.param<string>("pcd_topics", pcd_str, "/filtered_points"); 
  private_nh.param<string>("pose_topics", pose_str, "/ndt_pose"); 
  private_nh.param<string>("imu_topics", imu_str, "/imu_raw"); 
  private_nh.param<string>("compressed_img_topics", compressed_img_str, "/camera/image_raw/compressed"); 
  private_nh.param<string>("img_topics", img_str, ""); 
  private_nh.param<string>("marker_array_topics", marker_array_str, "/detection_range,/local_waypoints_mark,/global_waypoints_mark"); 
  private_nh.param<string>("marker_topics", marker_str, "/next_target_mark,/trajectory_circle_mark"); 
  private_nh.param<string>("can_topics", can_str, "/can_bus_dbw/can_rx"); 
  private_nh.param<string>("can_topics", can_str, "/can_bus_dbw/can_rx"); 
  private_nh.param<string>("nmea_topics", nmea_str, "/nmea_sentence");   
  private_nh.param<string>("auto_commit", auto_commit, "true");
  private_nh.param<string>("max_msg_size", max_msg_size, "1000000000"); 
  private_nh.param<string>("max_copy_msg_size", max_copy_msg_size, "1000000000"); 
  private_nh.param<string>("fetch_max_msg_size", fetch_max_msg_size, "2147483135");
  private_nh.param<string>("recv_max_msg_size", recv_max_msg_size, "2147483647"); 
  private_nh.param<string>("queue_max_msg_size", queue_max_msg_size, "2097151"); 
  private_nh.param<string>("queue_buffer_max_msg_size", queue_buffer_max_msg_size, "2097151");
  private_nh.param<string>("socket_send_buffer_size", socket_send_buffer_size, "0");
  private_nh.param<string>("socket_recv_buffer_size", socket_recv_buffer_size, "0");
  
  /*
   * Create configuration objects
   */
  Conf *conf = Conf::create(Conf::CONF_GLOBAL);
  Conf *tconf = Conf::create(Conf::CONF_TOPIC);
  
  /*
   * Set configuration properties
   */
  conf->set("metadata.broker.list", brokers, errstr);
  conf->set("group.id", group_id, errstr);
  // Disable auto commit
  conf->set("enable.auto.commit", auto_commit, errstr);  
  conf->set("message.max.bytes", max_msg_size, errstr);
  conf->set("message.copy.max.bytes", max_copy_msg_size, errstr);
  conf->set("receive.message.max.bytes", recv_max_msg_size, errstr);
  conf->set("fetch.message.max.bytes", fetch_max_msg_size, errstr);
  conf->set("queued.max.messages.kbytes", queue_max_msg_size, errstr);
  conf->set("queue.buffering.max.kbytes", queue_buffer_max_msg_size, errstr);
  conf->set("socket.send.buffer.bytes", socket_send_buffer_size, errstr);
  conf->set("socket.receive.buffer.bytes", socket_recv_buffer_size, errstr);  
  
  
  /*
   * Get the partition we want to write to. If no partition is provided, this will be
   * an unassigned one
   */
  if (partition_value != -1) {
    partition = partition_value;
  } 
  
  // Stop processing on SIGINT
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);
  
  
  /*
   *  tf2_msgs::TFMessage
   *
   */
  if (tf_str.size() != 0) {
    boost::split(tf_topics, tf_str, [](char c){return c == ',';});
    for (size_t i=0; i<tf_topics.size(); i++) {       
      string ros_topic = tf_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;
      ros::Publisher pub;  
      cout << tf_topics[i] << endl;
      pub = nh.advertise<tf2_msgs::TFMessage>(tf_topics[i], 1);     

      /*
       * Create consumer using accumulated global configuration.
       */
      Consumer *consumer = Consumer::create(conf, errstr);
      if (!consumer) {
        cerr << "Failed to create tf consumer: " << errstr << endl;
        exit(1);
      }
      tf_consumers.push_back(unique_ptr<Consumer>(consumer));
      cout << "% Created tf consumer " << consumer->name() << endl;

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(consumer, topic_name,
                 tconf, errstr);
      if (!topic) {
        cerr << "Failed to create tf topic: " << errstr << endl;
        exit(1);
      }
      kafka_tf_topics.push_back(unique_ptr<Topic>(topic));    

      /*
       * Start consumer for topic+partition at start offset
       */
      ErrorCode resp = consumer->start(topic, partition, start_offset);
      if (resp != RdKafka::ERR_NO_ERROR) {
        std::cerr << "Failed to start tf consumer: " <<
          RdKafka::err2str(resp) << std::endl;
        exit(1);
      }       

      tf_publishers.push_back(pub);

      // create publisher threads:
      pthread_t pthread = i;    
      bool tf_running = true;        
      if(pthread_create(&pthread, nullptr,(&tf_consumer_thread), (void *)i) != 0)
      {
         std::perror("tf pthread_create");
      }
      tf_runnings.push_back(tf_running);
      tf_pthreads.push_back(pthread);
    }
  }
  
  /*
   *  sensor_msgs::PointCloud2
   *
   */
  if (pcd_str.size() != 0) {
    boost::split(pcd_topics, pcd_str, [](char c){return c == ',';});
    for (size_t i=0; i<pcd_topics.size(); i++) {       
      string ros_topic = pcd_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;
      ros::Publisher pub;  
      cout << pcd_topics[i] << endl;
      pub = nh.advertise<sensor_msgs::PointCloud2>(pcd_topics[i], 1);     

      /*
       * Create consumer using accumulated global configuration.
       */
      Consumer *consumer = Consumer::create(conf, errstr);
      if (!consumer) {
        cerr << "Failed to create pcd consumer: " << errstr << endl;
        exit(1);
      }
      pcd_consumers.push_back(unique_ptr<Consumer>(consumer));
      cout << "% Created pcd consumer " << consumer->name() << endl;

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(consumer, topic_name,
                 tconf, errstr);
      if (!topic) {
        cerr << "Failed to create pcd topic: " << errstr << endl;
        exit(1);
      }
      kafka_pcd_topics.push_back(unique_ptr<Topic>(topic));    

      /*
       * Start consumer for topic+partition at start offset
       */
      ErrorCode resp = consumer->start(topic, partition, start_offset);
      if (resp != RdKafka::ERR_NO_ERROR) {
        std::cerr << "Failed to start pcd consumer: " <<
          RdKafka::err2str(resp) << std::endl;
        exit(1);
      }       

      pcd_publishers.push_back(pub);

      // create publisher threads:
      pthread_t pthread = i;    
      bool pcd_running = true;  
      
      if(pthread_create(&pthread, nullptr,(&pcd_consumer_thread), (void *)i) != 0)
      {
         std::perror("pcd pthread_create");
      }
      pcd_runnings.push_back(pcd_running);
      pcd_pthreads.push_back(pthread);
    }
  }
  
  /*
   *  geometry_msgs::PoseStamped
   *
   */
  if (pose_str.size() != 0) {
    boost::split(pose_topics, pose_str, [](char c){return c == ',';});
    for (size_t i=0; i<pose_topics.size(); i++) {       
      string ros_topic = pose_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;
      ros::Publisher pub;  
      cout << pose_topics[i] << endl;
      pub = nh.advertise<geometry_msgs::PoseStamped>(pose_topics[i], 1);     

      /*
       * Create consumer using accumulated global configuration.
       */
      Consumer *consumer = Consumer::create(conf, errstr);
      if (!consumer) {
        cerr << "Failed to create pose consumer: " << errstr << endl;
        exit(1);
      }
      pose_consumers.push_back(unique_ptr<Consumer>(consumer));
      cout << "% Created pose consumer " << consumer->name() << endl;

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(consumer, topic_name,
                 tconf, errstr);
      if (!topic) {
        cerr << "Failed to create pose topic: " << errstr << endl;
        exit(1);
      }
      kafka_pose_topics.push_back(unique_ptr<Topic>(topic));    

      /*
       * Start consumer for topic+partition at start offset
       */
      ErrorCode resp = consumer->start(topic, partition, start_offset);
      if (resp != RdKafka::ERR_NO_ERROR) {
        std::cerr << "Failed to start pose consumer: " <<
          RdKafka::err2str(resp) << std::endl;
        exit(1);
      }       

      pose_publishers.push_back(pub);

      // create publisher threads:
      pthread_t pthread = i;    
      bool pose_running = true;  
      
      if(pthread_create(&pthread, nullptr,(&pose_consumer_thread), (void *)i) != 0)
      {
         std::perror("pose pthread_create");
      }
      pose_runnings.push_back(pose_running);
      pose_pthreads.push_back(pthread);
    }
  }
  
  /*
   *  sensor_msgs::Imu
   *
   */
  if (imu_str.size() != 0) {
    boost::split(imu_topics, imu_str, [](char c){return c == ',';});
    for (size_t i=0; i<imu_topics.size(); i++) {       
      string ros_topic = imu_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;
      ros::Publisher pub;  
      cout << imu_topics[i] << endl;
      pub = nh.advertise<sensor_msgs::Imu>(imu_topics[i], 1);     

      /*
       * Create consumer using accumulated global configuration.
       */
      Consumer *consumer = Consumer::create(conf, errstr);
      if (!consumer) {
        cerr << "Failed to create imu consumer: " << errstr << endl;
        exit(1);
      }
      imu_consumers.push_back(unique_ptr<Consumer>(consumer));
      cout << "% Created imu consumer " << consumer->name() << endl;

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(consumer, topic_name,
                 tconf, errstr);
      if (!topic) {
        cerr << "Failed to create imu topic: " << errstr << endl;
        exit(1);
      }
      kafka_imu_topics.push_back(unique_ptr<Topic>(topic));    

      /*
       * Start consumer for topic+partition at start offset
       */
      ErrorCode resp = consumer->start(topic, partition, start_offset);
      if (resp != RdKafka::ERR_NO_ERROR) {
        std::cerr << "Failed to start imu consumer: " <<
          RdKafka::err2str(resp) << std::endl;
        exit(1);
      }       

      imu_publishers.push_back(pub);

      // create publisher threads:
      pthread_t pthread = i;    
      bool imu_running = true;  
      
      if(pthread_create(&pthread, nullptr,(&imu_consumer_thread), (void *)i) != 0)
      {
         std::perror("imu pthread_create");
      }
      imu_runnings.push_back(imu_running);
      imu_pthreads.push_back(pthread);
    }
  }
  
  /*
   *  sensor_msgs::Image
   *
   */
  if (img_str.size() != 0) {
    boost::split(img_topics, img_str, [](char c){return c == ',';});
    for (size_t i=0; i<img_topics.size(); i++) {       
      string ros_topic = img_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;
      ros::Publisher pub;  
      cout << img_topics[i] << endl;
      pub = nh.advertise<sensor_msgs::Image>(img_topics[i], 1);     

      /*
       * Create consumer using accumulated global configuration.
       */
      Consumer *consumer = Consumer::create(conf, errstr);
      if (!consumer) {
        cerr << "Failed to create img consumer: " << errstr << endl;
        exit(1);
      }
      img_consumers.push_back(unique_ptr<Consumer>(consumer));
      cout << "% Created img consumer " << consumer->name() << endl;

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(consumer, topic_name,
                 tconf, errstr);
      if (!topic) {
        cerr << "Failed to create img topic: " << errstr << endl;
        exit(1);
      }
      kafka_img_topics.push_back(unique_ptr<Topic>(topic));    

      /*
       * Start consumer for topic+partition at start offset
       */
      ErrorCode resp = consumer->start(topic, partition, start_offset);
      if (resp != RdKafka::ERR_NO_ERROR) {
        std::cerr << "Failed to start img consumer: " <<
          RdKafka::err2str(resp) << std::endl;
        exit(1);
      }       

      img_publishers.push_back(pub);

      // create publisher threads:
      pthread_t pthread = i;    
      bool img_running = true;  
      
      if(pthread_create(&pthread, nullptr,(&img_consumer_thread), (void *)i) != 0)
      {
         std::perror("img pthread_create");
      }
      img_runnings.push_back(img_running);
      img_pthreads.push_back(pthread);
    }
  }
  
  /*
   *  sensor_msgs::CompressedImage
   *
   */
  if (compressed_img_str.size() != 0) {
    boost::split(compressed_img_topics, compressed_img_str, [](char c){return c == ',';});
    for (size_t i=0; i<compressed_img_topics.size(); i++) {       
      string ros_topic = compressed_img_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;
      ros::Publisher pub;  
      cout << compressed_img_topics[i] << endl;
      pub = nh.advertise<sensor_msgs::CompressedImage>(compressed_img_topics[i], 1);     

      /*
       * Create consumer using accumulated global configuration.
       */
      Consumer *consumer = Consumer::create(conf, errstr);
      if (!consumer) {
        cerr << "Failed to create compressed_img consumer: " << errstr << endl;
        exit(1);
      }
      compressed_img_consumers.push_back(unique_ptr<Consumer>(consumer));
      cout << "% Created compressed_img consumer " << consumer->name() << endl;

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(consumer, topic_name,
                 tconf, errstr);
      if (!topic) {
        cerr << "Failed to create compressed_img topic: " << errstr << endl;
        exit(1);
      }
      kafka_compressed_img_topics.push_back(unique_ptr<Topic>(topic));    

      /*
       * Start consumer for topic+partition at start offset
       */
      ErrorCode resp = consumer->start(topic, partition, start_offset);
      if (resp != RdKafka::ERR_NO_ERROR) {
        std::cerr << "Failed to start compressed_img consumer: " <<
          RdKafka::err2str(resp) << std::endl;
        exit(1);
      }       

      compressed_img_publishers.push_back(pub);

      // create publisher threads:
      pthread_t pthread = i;    
      bool compressed_img_running = true;  
      
      if(pthread_create(&pthread, nullptr,(&compressed_img_consumer_thread), (void *)i) != 0)
      {
         std::perror("compressed_img pthread_create");
      }
      compressed_img_runnings.push_back(compressed_img_running);
      compressed_img_pthreads.push_back(pthread);
    }
  }  
  
  /*
   *  visualization_msgs::MarkerArray
   *
   */
  if (marker_array_str.size() != 0) {
    boost::split(marker_array_topics, marker_array_str, [](char c){return c == ',';});
    for (size_t i=0; i<marker_array_topics.size(); i++) {       
      string ros_topic = marker_array_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;
      ros::Publisher pub;  
      cout << marker_array_topics[i] << endl;
      pub = nh.advertise<visualization_msgs::MarkerArray>(marker_array_topics[i], 1);     

      /*
       * Create consumer using accumulated global configuration.
       */
      Consumer *consumer = Consumer::create(conf, errstr);
      if (!consumer) {
        cerr << "Failed to create marker_array consumer: " << errstr << endl;
        exit(1);
      }
      marker_array_consumers.push_back(unique_ptr<Consumer>(consumer));
      cout << "% Created marker_array consumer " << consumer->name() << endl;

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(consumer, topic_name,
                 tconf, errstr);
      if (!topic) {
        cerr << "Failed to create marker_array topic: " << errstr << endl;
        exit(1);
      }
      kafka_marker_array_topics.push_back(unique_ptr<Topic>(topic));    

      /*
       * Start consumer for topic+partition at start offset
       */
      ErrorCode resp = consumer->start(topic, partition, start_offset);
      if (resp != RdKafka::ERR_NO_ERROR) {
        std::cerr << "Failed to start marker_array consumer: " <<
          RdKafka::err2str(resp) << std::endl;
        exit(1);
      }       

      marker_array_publishers.push_back(pub);

      // create publisher threads:
      pthread_t pthread = i;    
      bool marker_array_running = true;  
      
      if(pthread_create(&pthread, nullptr,(&marker_array_consumer_thread), (void *)i) != 0)
      {
         std::perror("marker_array pthread_create");
      }
      marker_array_runnings.push_back(marker_array_running);
      marker_array_pthreads.push_back(pthread);
    }
  }
    
  /*
   *  visualization_msgs::Marker
   *
   */
  if (marker_str.size() != 0) {
    boost::split(marker_topics, marker_str, [](char c){return c == ',';});
    for (size_t i=0; i<marker_topics.size(); i++) {       
      string ros_topic = marker_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;
      ros::Publisher pub;  
      cout << marker_topics[i] << endl;
      pub = nh.advertise<visualization_msgs::Marker>(marker_topics[i], 1);     

      /*
       * Create consumer using accumulated global configuration.
       */
      Consumer *consumer = Consumer::create(conf, errstr);
      if (!consumer) {
        cerr << "Failed to create marker consumer: " << errstr << endl;
        exit(1);
      }
      marker_consumers.push_back(unique_ptr<Consumer>(consumer));
      cout << "% Created marker consumer " << consumer->name() << endl;

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(consumer, topic_name,
                 tconf, errstr);
      if (!topic) {
        cerr << "Failed to create marker topic: " << errstr << endl;
        exit(1);
      }
      kafka_marker_topics.push_back(unique_ptr<Topic>(topic));    

      /*
       * Start consumer for topic+partition at start offset
       */
      ErrorCode resp = consumer->start(topic, partition, start_offset);
      if (resp != RdKafka::ERR_NO_ERROR) {
        std::cerr << "Failed to start marker consumer: " <<
          RdKafka::err2str(resp) << std::endl;
        exit(1);
      }       

      marker_publishers.push_back(pub);

      // create publisher threads:
      pthread_t pthread = i;    
      bool marker_running = true;  
      
      if(pthread_create(&pthread, nullptr,(&marker_consumer_thread), (void *)i) != 0)
      {
         std::perror("marker pthread_create");
      }
      marker_runnings.push_back(marker_running);
      marker_pthreads.push_back(pthread);
    }
  }
    
  /*
   *  can_msgs::Frame
   *
   */
  if (can_str.size() != 0) {
    boost::split(can_topics, can_str, [](char c){return c == ',';});
    for (size_t i=0; i<can_topics.size(); i++) {       
      string ros_topic = can_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;
      ros::Publisher pub;  
      cout << can_topics[i] << endl;
      pub = nh.advertise<can_msgs::Frame>(can_topics[i], 1);     

      /*
       * Create consumer using accumulated global configuration.
       */
      Consumer *consumer = Consumer::create(conf, errstr);
      if (!consumer) {
        cerr << "Failed to create can consumer: " << errstr << endl;
        exit(1);
      }
      can_consumers.push_back(unique_ptr<Consumer>(consumer));
      cout << "% Created can consumer " << consumer->name() << endl;

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(consumer, topic_name,
                 tconf, errstr);
      if (!topic) {
        cerr << "Failed to create can topic: " << errstr << endl;
        exit(1);
      }
      kafka_can_topics.push_back(unique_ptr<Topic>(topic));    

      /*
       * Start consumer for topic+partition at start offset
       */
      ErrorCode resp = consumer->start(topic, partition, start_offset);
      if (resp != RdKafka::ERR_NO_ERROR) {
        std::cerr << "Failed to start can consumer: " <<
          RdKafka::err2str(resp) << std::endl;
        exit(1);
      }       

      can_publishers.push_back(pub);

      // create publisher threads:
      pthread_t pthread = i;    
      bool can_running = true;  
      
      if(pthread_create(&pthread, nullptr,(&can_consumer_thread), (void *)i) != 0)
      {
         std::perror("can pthread_create");
      }
      can_runnings.push_back(can_running);
      can_pthreads.push_back(pthread);
    }
  } 
  
  /*
   * nmea_msgs::Sentence
   *
   */
  if (nmea_str.size() != 0) {
    boost::split(nmea_topics, nmea_str, [](char c){return c == ',';});
    for (size_t i=0; i<nmea_topics.size(); i++) {       
      string ros_topic = nmea_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;
      ros::Publisher pub;  
      cout << nmea_topics[i] << endl;
      pub = nh.advertise<nmea_msgs::Sentence>(nmea_topics[i], 1);     

      /*
       * Create consumer using accumulated global configuration.
       */
      Consumer *consumer = Consumer::create(conf, errstr);
      if (!consumer) {
        cerr << "Failed to create nmea consumer: " << errstr << endl;
        exit(1);
      }
      nmea_consumers.push_back(unique_ptr<Consumer>(consumer));
      cout << "% Created nmea consumer " << consumer->name() << endl;

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(consumer, topic_name,
                 tconf, errstr);
      if (!topic) {
        cerr << "Failed to create nmea topic: " << errstr << endl;
        exit(1);
      }
      kafka_nmea_topics.push_back(unique_ptr<Topic>(topic));    

      /*
       * Start consumer for topic+partition at start offset
       */
      ErrorCode resp = consumer->start(topic, partition, start_offset);
      if (resp != RdKafka::ERR_NO_ERROR) {
        std::cerr << "Failed to start nmea consumer: " <<
          RdKafka::err2str(resp) << std::endl;
        exit(1);
      }       

      nmea_publishers.push_back(pub);

      // create publisher threads:
      pthread_t pthread = i;    
      bool nmea_running = true;  
      
      if(pthread_create(&pthread, nullptr,(&nmea_consumer_thread), (void *)i) != 0)
      {
         std::perror("nmea pthread_create");
      }
      nmea_runnings.push_back(nmea_running);
      nmea_pthreads.push_back(pthread);
    }
  } 
    
  ros::spin();
  
  for (size_t i=0; i < tf_pthreads.size(); i++) 
    pthread_join(tf_pthreads[i], nullptr);
  for (size_t i=0; i < pcd_pthreads.size(); i++) 
    pthread_join(pcd_pthreads[i], nullptr);
  for (size_t i=0; i < pose_pthreads.size(); i++) 
    pthread_join(pose_pthreads[i], nullptr);    
  for (size_t i=0; i < imu_pthreads.size(); i++) 
    pthread_join(imu_pthreads[i], nullptr);
  for (size_t i=0; i < compressed_img_pthreads.size(); i++) 
    pthread_join(compressed_img_pthreads[i], nullptr);    
  for (size_t i=0; i < img_pthreads.size(); i++) 
    pthread_join(img_pthreads[i], nullptr);
  for (size_t i=0; i < marker_array_pthreads.size(); i++) 
    pthread_join(marker_array_pthreads[i], nullptr);
  for (size_t i=0; i < marker_pthreads.size(); i++) 
    pthread_join(marker_pthreads[i], nullptr);
  for (size_t i=0; i < can_pthreads.size(); i++) 
    pthread_join(can_pthreads[i], nullptr);
  for (size_t i=0; i < nmea_pthreads.size(); i++) 
    pthread_join(nmea_pthreads[i], nullptr);

  return 0;
  
}  
#include <ros/ros.h>
#include <tf2_msgs/TFMessage.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Quaternion.h>
#include <geometry_msgs/Vector3.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/CompressedImage.h>
#include <sensor_msgs/PointCloud2.h>
#include <visualization_msgs/MarkerArray.h>
#include <visualization_msgs/Marker.h>
#include <can_msgs/Frame.h>
#include <nmea_msgs/Sentence.h>

#include <stdexcept>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <librdkafka/rdkafkacpp.h>

using std::string;
using std::vector;
using std::unique_ptr;
using std::exception;
using std::getline;
using std::cin;
using std::cout;
using std::endl;
using std::cerr;

using RdKafka::Producer;
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

static size_t i;

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


static vector<unique_ptr<Producer>> tf_producers;
static vector<unique_ptr<Producer>> pcd_producers;
static vector<unique_ptr<Producer>> pose_producers;
static vector<unique_ptr<Producer>> imu_producers;
static vector<unique_ptr<Producer>> compressed_img_producers;
static vector<unique_ptr<Producer>> img_producers;
static vector<unique_ptr<Producer>> marker_array_producers;
static vector<unique_ptr<Producer>> marker_producers;
static vector<unique_ptr<Producer>> can_producers;
static vector<unique_ptr<Producer>> nmea_producers;

static vector<ros::Subscriber> tf_subs;
static vector<ros::Subscriber> pcd_subs;
static vector<ros::Subscriber> pose_subs;
static vector<ros::Subscriber> imu_subs;
static vector<ros::Subscriber> compressed_img_subs;
static vector<ros::Subscriber> img_subs;
static vector<ros::Subscriber> marker_array_subs;
static vector<ros::Subscriber> marker_subs;
static vector<ros::Subscriber> can_subs;
static vector<ros::Subscriber> nmea_subs;

/*
 * Define callback function for tf.
 */      
void tf_cb(const tf2_msgs::TFMessage::ConstPtr& input, size_t i) {

  /*
   * Produce message
   */  

  cout << "% produce msg. producer " << tf_producers[i].get()->name() << endl;    

  uint32_t serial_size = ros::serialization::serializationLength(*input);
  boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);
  ros::serialization::OStream stream(buffer.get(), serial_size);
  ros::serialization::serialize(stream, *input);


  RdKafka::ErrorCode resp =
        tf_producers[i].get()->produce(kafka_tf_topics[i].get(), partition,
                          Producer::RK_MSG_COPY, // Copy payload
                          (void *)buffer.get(), serial_size,      
                          NULL, NULL);  

  if (resp != ERR_NO_ERROR)
    std::cerr << "% Produce failed: " <<
      RdKafka::err2str(resp) << std::endl;
  else
    std::cerr << "% Produced message (" << serial_size << " bytes)" <<
      std::endl;

  tf_producers[i]->poll(0);

}


/*
 * Define callback function for pcd.
 */
void pcd_cb(const sensor_msgs::PointCloud2::ConstPtr& input, size_t i) {
  /*
   * Produce message
   */  

  cout << "% produce msg. producer " << pcd_producers[i].get()->name() << endl;    

  uint32_t serial_size = ros::serialization::serializationLength(*input);
  boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);
  ros::serialization::OStream stream(buffer.get(), serial_size);
  ros::serialization::serialize(stream, *input);


  RdKafka::ErrorCode resp =
        pcd_producers[i].get()->produce(kafka_pcd_topics[i].get(), partition,
                          Producer::RK_MSG_COPY, // Copy payload
                          (void *)buffer.get(), serial_size,      
                          NULL, NULL);  

  if (resp != ERR_NO_ERROR)
    std::cerr << "% Produce failed: " <<
      RdKafka::err2str(resp) << std::endl;
  else
    std::cerr << "% Produced message (" << serial_size << " bytes)" <<
      std::endl;

  pcd_producers[i]->poll(0);
}

/*
 * Define callback function for pose.
 */
void pose_cb(const geometry_msgs::PoseStamped::ConstPtr& input, size_t i) {
  /*
   * Produce message
   */  

  cout << "% produce msg. producer " << pose_producers[i].get()->name() << endl;    

  uint32_t serial_size = ros::serialization::serializationLength(*input);
  boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);
  ros::serialization::OStream stream(buffer.get(), serial_size);
  ros::serialization::serialize(stream, *input);


  RdKafka::ErrorCode resp =
        pose_producers[i].get()->produce(kafka_pose_topics[i].get(), partition,
                          Producer::RK_MSG_COPY, // Copy payload
                          (void *)buffer.get(), serial_size,      
                          NULL, NULL);  

  if (resp != ERR_NO_ERROR)
    std::cerr << "% Produce failed: " <<
      RdKafka::err2str(resp) << std::endl;
  else
    std::cerr << "% Produced message (" << serial_size << " bytes)" <<
      std::endl;

  pose_producers[i]->poll(0);
}

/*
 * Define callback function for imu.
 */
void imu_cb(const sensor_msgs::Imu::ConstPtr& input, size_t i) {
  /*
   * Produce message
   */  

  cout << "% produce msg. producer " << imu_producers[i].get()->name() << endl;    

  uint32_t serial_size = ros::serialization::serializationLength(*input);
  boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);
  ros::serialization::OStream stream(buffer.get(), serial_size);
  ros::serialization::serialize(stream, *input);


  RdKafka::ErrorCode resp =
        imu_producers[i].get()->produce(kafka_imu_topics[i].get(), partition,
                          Producer::RK_MSG_COPY, // Copy payload
                          (void *)buffer.get(), serial_size,      
                          NULL, NULL);  

  if (resp != ERR_NO_ERROR)
    std::cerr << "% Produce failed: " <<
      RdKafka::err2str(resp) << std::endl;
  else
    std::cerr << "% Produced message (" << serial_size << " bytes)" <<
      std::endl;

  imu_producers[i]->poll(0);
}

/*
 * Define callback function for compressed_img.
 */
void compressed_img_cb(const sensor_msgs::CompressedImage::ConstPtr& input, size_t i) {
  /*
   * Produce message
   */  

  cout << "% produce msg. producer " << compressed_img_producers[i].get()->name() << endl;    

  uint32_t serial_size = ros::serialization::serializationLength(*input);
  boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);
  ros::serialization::OStream stream(buffer.get(), serial_size);
  ros::serialization::serialize(stream, *input);


  RdKafka::ErrorCode resp =
        compressed_img_producers[i].get()->produce(kafka_compressed_img_topics[i].get(), partition,
                          Producer::RK_MSG_COPY, // Copy payload
                          (void *)buffer.get(), serial_size,      
                          NULL, NULL);  

  if (resp != ERR_NO_ERROR)
    std::cerr << "% Produce failed: " <<
      RdKafka::err2str(resp) << std::endl;
  else
    std::cerr << "% Produced message (" << serial_size << " bytes)" <<
      std::endl;

  compressed_img_producers[i]->poll(0);
}

/*
 * Define callback function for img.
 */
void img_cb(const sensor_msgs::Image::ConstPtr& input, size_t i) {
  /*
   * Produce message
   */  

  cout << "% produce msg. producer " << img_producers[i].get()->name() << endl;    

  uint32_t serial_size = ros::serialization::serializationLength(*input);
  boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);
  ros::serialization::OStream stream(buffer.get(), serial_size);
  ros::serialization::serialize(stream, *input);


  RdKafka::ErrorCode resp =
        img_producers[i].get()->produce(kafka_img_topics[i].get(), partition,
                          Producer::RK_MSG_COPY, // Copy payload
                          (void *)buffer.get(), serial_size,      
                          NULL, NULL);  

  if (resp != ERR_NO_ERROR)
    std::cerr << "% Produce failed: " <<
      RdKafka::err2str(resp) << std::endl;
  else
    std::cerr << "% Produced message (" << serial_size << " bytes)" <<
      std::endl;

  img_producers[i]->poll(0);
}

/*
 * Define callback function for marker_array.
 */
void marker_array_cb(const visualization_msgs::MarkerArray::ConstPtr& input, size_t i) {
  /*
   * Produce message
   */  

  cout << "% produce msg. producer " << marker_array_producers[i].get()->name() << endl;    

  uint32_t serial_size = ros::serialization::serializationLength(*input);
  boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);
  ros::serialization::OStream stream(buffer.get(), serial_size);
  ros::serialization::serialize(stream, *input);


  RdKafka::ErrorCode resp =
        marker_array_producers[i].get()->produce(kafka_marker_array_topics[i].get(), partition,
                          Producer::RK_MSG_COPY, // Copy payload
                          (void *)buffer.get(), serial_size,      
                          NULL, NULL);  

  if (resp != ERR_NO_ERROR)
    std::cerr << "% Produce failed: " <<
      RdKafka::err2str(resp) << std::endl;
  else
    std::cerr << "% Produced message (" << serial_size << " bytes)" <<
      std::endl;

  marker_array_producers[i]->poll(0);
}

/*
 * Define callback function for marker.
 */
void marker_cb(const visualization_msgs::Marker::ConstPtr& input, size_t i) {
  /*
   * Produce message
   */  

  cout << "% produce msg. producer " << marker_producers[i].get()->name() << endl;    

  uint32_t serial_size = ros::serialization::serializationLength(*input);
  boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);
  ros::serialization::OStream stream(buffer.get(), serial_size);
  ros::serialization::serialize(stream, *input);


  RdKafka::ErrorCode resp =
        marker_producers[i].get()->produce(kafka_marker_topics[i].get(), partition,
                          Producer::RK_MSG_COPY, // Copy payload
                          (void *)buffer.get(), serial_size,      
                          NULL, NULL);  

  if (resp != ERR_NO_ERROR)
    std::cerr << "% Produce failed: " <<
      RdKafka::err2str(resp) << std::endl;
  else
    std::cerr << "% Produced message (" << serial_size << " bytes)" <<
      std::endl;

  marker_producers[i]->poll(0);
}

/*
 * Define callback function for can.
 */
void can_cb(const can_msgs::Frame::ConstPtr& input, size_t i) {
  /*
   * Produce message
   */  

  cout << "% produce msg. producer " << can_producers[i].get()->name() << endl;    

  uint32_t serial_size = ros::serialization::serializationLength(*input);
  boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);
  ros::serialization::OStream stream(buffer.get(), serial_size);
  ros::serialization::serialize(stream, *input);


  RdKafka::ErrorCode resp =
        can_producers[i].get()->produce(kafka_can_topics[i].get(), partition,
                          Producer::RK_MSG_COPY, // Copy payload
                          (void *)buffer.get(), serial_size,      
                          NULL, NULL);  

  if (resp != ERR_NO_ERROR)
    std::cerr << "% Produce failed: " <<
      RdKafka::err2str(resp) << std::endl;
  else
    std::cerr << "% Produced message (" << serial_size << " bytes)" <<
      std::endl;

  can_producers[i]->poll(0);
}

/*
 * Define callback function for nmea.
 */
void nmea_cb(const nmea_msgs::Sentence::ConstPtr& input, size_t i) {
  /*
   * Produce message
   */  

  cout << "% produce msg. producer " << nmea_producers[i].get()->name() << endl;    

  uint32_t serial_size = ros::serialization::serializationLength(*input);
  boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);
  ros::serialization::OStream stream(buffer.get(), serial_size);
  ros::serialization::serialize(stream, *input);


  RdKafka::ErrorCode resp =
        nmea_producers[i].get()->produce(kafka_nmea_topics[i].get(), partition,
                          Producer::RK_MSG_COPY, // Copy payload
                          (void *)buffer.get(), serial_size,      
                          NULL, NULL);  

  if (resp != ERR_NO_ERROR)
    std::cerr << "% Produce failed: " <<
      RdKafka::err2str(resp) << std::endl;
  else
    std::cerr << "% Produced message (" << serial_size << " bytes)" <<
      std::endl;

  nmea_producers[i]->poll(0);
}

int main(int argc, char* argv[]) {
  int32_t partition_value;
  ros::init(argc, argv, "kafka_producer");
  
  ros::NodeHandle nh;
  ros::NodeHandle private_nh("~");
  
  private_nh.param<string>("brokers", brokers, "localhost:9092");  
  private_nh.param<int>("partition", partition_value, 0);
  private_nh.param<string>("vehicle_id", vehicle_id, "vehicle");
  private_nh.param<string>("tf_topics", tf_str, "/tf_changes"); 
  private_nh.param<string>("pcd_topics", pcd_str, "/filtered_points"); 
  private_nh.param<string>("pose_topics", pose_str, "/ndt_pose"); 
  private_nh.param<string>("imu_topics", imu_str, "/imu_raw"); 
  private_nh.param<string>("compressed_img_topics", compressed_img_str, "/camera/image_raw/compressed"); 
  private_nh.param<string>("img_topics", img_str, ""); 
  private_nh.param<string>("marker_array_topics", marker_array_str, "/detection_range,/local_waypoints_mark,/global_waypoints_mark"); 
  private_nh.param<string>("marker_topics", marker_str, "/next_target_mark,/trajectory_circle_mark"); 
  private_nh.param<string>("can_topics", can_str, "/can_bus_dbw/can_rx");
  private_nh.param<string>("nmea_topics", nmea_str, "/nmea_sentence");   
  private_nh.param<string>("max_msg_size", max_msg_size, "1000000000"); 
  private_nh.param<string>("max_copy_msg_size", max_copy_msg_size, "1000000000"); 
  private_nh.param<string>("recv_max_msg_size", recv_max_msg_size, "2147483647"); 
  private_nh.param<string>("fetch_max_msg_size", fetch_max_msg_size, "2147483135");
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
  
  /*
   *  tf2_msgs::TFMessage
   *
   */
  if (tf_str.size() != 0) {
    boost::split(tf_topics, tf_str, [](char c){return c == ',';});
    
    for (i=0; i<tf_topics.size(); i++) {
      string ros_topic = tf_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;    

      // Create a message builder and producer for corresponding topics  

      /*
       * Create producer using accumulated global configuration.
       */
      Producer *producer = Producer::create(conf, errstr);
      if (!producer) {
        cerr << "Failed to create producer: " << errstr << endl;
        exit(1);
      }
      tf_producers.push_back(unique_ptr<Producer>(producer));
      cout << "% Created producer " << producer->name() << endl;      

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(producer, topic_name, tconf, errstr);
      if (!topic) {
        cerr << "Failed to create topic: " << errstr << endl;
        exit(1);
      } 
      kafka_tf_topics.push_back(unique_ptr<Topic>(topic));    
      cout << "Producing messages into topic " << topic_name << endl;        
      
      ros::Subscriber tf_sub = nh.subscribe<tf2_msgs::TFMessage>(tf_topics[i], 1, boost::bind(tf_cb, _1, i)); 

      tf_subs.push_back(tf_sub);
    }
  }
  
  /*
   *  sensor_msgs::PointCloud2
   *
   */
  if (pcd_str.size() != 0) {
    boost::split(pcd_topics, pcd_str, [](char c){return c == ',';});
    
    for (i=0; i<pcd_topics.size(); i++) {
      string ros_topic = pcd_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;    

      // Create a message builder and producer for corresponding topics  

      /*
       * Create producer using accumulated global configuration.
       */
      Producer *producer = Producer::create(conf, errstr);
      if (!producer) {
        cerr << "Failed to create producer: " << errstr << endl;
        exit(1);
      }
      pcd_producers.push_back(unique_ptr<Producer>(producer));
      cout << "% Created producer " << producer->name() << endl;      

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(producer, topic_name, tconf, errstr);
      if (!topic) {
        cerr << "Failed to create topic: " << errstr << endl;
        exit(1);
      } 
      kafka_pcd_topics.push_back(unique_ptr<Topic>(topic));    
      cout << "Producing messages into topic " << topic_name << endl;  

      
      ros::Subscriber pcd_sub = nh.subscribe<sensor_msgs::PointCloud2>(pcd_topics[i], 1, boost::bind(pcd_cb, _1, i)); 

      pcd_subs.push_back(pcd_sub);
    }
  }
    
  /*
   *  geometry_msgs::PoseStamped
   *
   */
  if (pose_str.size() != 0) {
    boost::split(pose_topics, pose_str, [](char c){return c == ',';});
    
    for (i=0; i<pose_topics.size(); i++) {
      string ros_topic = pose_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;    

      // Create a message builder and producer for corresponding topics  

      /*
       * Create producer using accumulated global configuration.
       */
      Producer *producer = Producer::create(conf, errstr);
      if (!producer) {
        cerr << "Failed to create producer: " << errstr << endl;
        exit(1);
      }
      pose_producers.push_back(unique_ptr<Producer>(producer));
      cout << "% Created producer " << producer->name() << endl;      

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(producer, topic_name, tconf, errstr);
      if (!topic) {
        cerr << "Failed to create topic: " << errstr << endl;
        exit(1);
      } 
      kafka_pose_topics.push_back(unique_ptr<Topic>(topic));    
      cout << "Producing messages into topic " << topic_name << endl;       

      ros::Subscriber pose_sub = nh.subscribe<geometry_msgs::PoseStamped>(pose_topics[i], 1, boost::bind(pose_cb, _1, i)); 

      pose_subs.push_back(pose_sub);
    }
  }
  
  /*
   *  sensor_msgs::Imu
   *
   */
  if (imu_str.size() != 0) {
    boost::split(imu_topics, imu_str, [](char c){return c == ',';});
    
    for (i=0; i<imu_topics.size(); i++) {
      string ros_topic = imu_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;    

      // Create a message builder and producer for corresponding topics  

      /*
       * Create producer using accumulated global configuration.
       */
      Producer *producer = Producer::create(conf, errstr);
      if (!producer) {
        cerr << "Failed to create producer: " << errstr << endl;
        exit(1);
      }
      imu_producers.push_back(unique_ptr<Producer>(producer));
      cout << "% Created producer " << producer->name() << endl;      

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(producer, topic_name, tconf, errstr);
      if (!topic) {
        cerr << "Failed to create topic: " << errstr << endl;
        exit(1);
      } 
      kafka_imu_topics.push_back(unique_ptr<Topic>(topic));    
      cout << "Producing messages into topic " << topic_name << endl;  

      ros::Subscriber imu_sub = nh.subscribe<sensor_msgs::Imu>(imu_topics[i], 1, boost::bind(imu_cb, _1, i)); 

      imu_subs.push_back(imu_sub);
    }
  }
  
  /*
   *  sensor_msgs::Image
   *
   */
  if (img_str.size() != 0) {
    boost::split(img_topics, img_str, [](char c){return c == ',';});
    
    for (i=0; i<img_topics.size(); i++) {
      string ros_topic = img_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;    

      // Create a message builder and producer for corresponding topics  

      /*
       * Create producer using accumulated global configuration.
       */
      Producer *producer = Producer::create(conf, errstr);
      if (!producer) {
        cerr << "Failed to create producer: " << errstr << endl;
        exit(1);
      }
      img_producers.push_back(unique_ptr<Producer>(producer));
      cout << "% Created producer " << producer->name() << endl;      

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(producer, topic_name, tconf, errstr);
      if (!topic) {
        cerr << "Failed to create topic: " << errstr << endl;
        exit(1);
      } 
      kafka_img_topics.push_back(unique_ptr<Topic>(topic));    
      cout << "Producing messages into topic " << topic_name << endl;  

      ros::Subscriber img_sub = nh.subscribe<sensor_msgs::Image>(img_topics[i], 1, boost::bind(img_cb, _1, i)); 

      img_subs.push_back(img_sub);
    }
  }
  
  /*
   *  sensor_msgs::CompressedImage
   *
   */
  if (compressed_img_str.size() != 0) {
    boost::split(compressed_img_topics, compressed_img_str, [](char c){return c == ',';});
    
    for (i=0; i<compressed_img_topics.size(); i++) {
      string ros_topic = compressed_img_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;    

      // Create a message builder and producer for corresponding topics  

      /*
       * Create producer using accumulated global configuration.
       */
      Producer *producer = Producer::create(conf, errstr);
      if (!producer) {
        cerr << "Failed to create producer: " << errstr << endl;
        exit(1);
      }
      compressed_img_producers.push_back(unique_ptr<Producer>(producer));
      cout << "% Created producer " << producer->name() << endl;      

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(producer, topic_name, tconf, errstr);
      if (!topic) {
        cerr << "Failed to create topic: " << errstr << endl;
        exit(1);
      } 
      kafka_compressed_img_topics.push_back(unique_ptr<Topic>(topic));    
      cout << "Producing messages into topic " << topic_name << endl;  

      ros::Subscriber compressed_img_sub = nh.subscribe<sensor_msgs::CompressedImage>(compressed_img_topics[i], 1, boost::bind(compressed_img_cb, _1, i)); 

      compressed_img_subs.push_back(compressed_img_sub);
    }
  }
  
  /*
   *  visualization_msgs::MarkerArray
   *
   */
  if (marker_array_str.size() != 0) {
    boost::split(marker_array_topics, marker_array_str, [](char c){return c == ',';});
    
    for (i=0; i<marker_array_topics.size(); i++) {
      string ros_topic = marker_array_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;    

      // Create a message builder and producer for corresponding topics  

      /*
       * Create producer using accumulated global configuration.
       */
      Producer *producer = Producer::create(conf, errstr);
      if (!producer) {
        cerr << "Failed to create producer: " << errstr << endl;
        exit(1);
      }
      marker_array_producers.push_back(unique_ptr<Producer>(producer));
      cout << "% Created producer " << producer->name() << endl;      

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(producer, topic_name, tconf, errstr);
      if (!topic) {
        cerr << "Failed to create topic: " << errstr << endl;
        exit(1);
      } 
      kafka_marker_array_topics.push_back(unique_ptr<Topic>(topic));    
      cout << "Producing messages into topic " << topic_name << endl;  

      ros::Subscriber marker_array_sub = nh.subscribe<visualization_msgs::MarkerArray>(marker_array_topics[i], 1, boost::bind(marker_array_cb, _1, i)); 

      marker_array_subs.push_back(marker_array_sub);
    }
  }
  
  /*
   *  visualization_msgs::Marker
   *
   */
  if (marker_str.size() != 0) {
    boost::split(marker_topics, marker_str, [](char c){return c == ',';});
    
    for (i=0; i<marker_topics.size(); i++) {
      string ros_topic = marker_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;    

      // Create a message builder and producer for corresponding topics  

      /*
       * Create producer using accumulated global configuration.
       */
      Producer *producer = Producer::create(conf, errstr);
      if (!producer) {
        cerr << "Failed to create producer: " << errstr << endl;
        exit(1);
      }
      marker_producers.push_back(unique_ptr<Producer>(producer));
      cout << "% Created producer " << producer->name() << endl;      

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(producer, topic_name, tconf, errstr);
      if (!topic) {
        cerr << "Failed to create topic: " << errstr << endl;
        exit(1);
      } 
      kafka_marker_topics.push_back(unique_ptr<Topic>(topic));    
      cout << "Producing messages into topic " << topic_name << endl;  

      ros::Subscriber marker_sub = nh.subscribe<visualization_msgs::Marker>(marker_topics[i], 1, boost::bind(marker_cb, _1, i)); 

      marker_subs.push_back(marker_sub);
    }
  }
  
  /*
   *  can_msgs::Frame
   *
   */
  if (can_str.size() != 0) {
    boost::split(can_topics, can_str, [](char c){return c == ',';});
    
    for (i=0; i<can_topics.size(); i++) {
      string ros_topic = can_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;    

      // Create a message builder and producer for corresponding topics  

      /*
       * Create producer using accumulated global configuration.
       */
      Producer *producer = Producer::create(conf, errstr);
      if (!producer) {
        cerr << "Failed to create producer: " << errstr << endl;
        exit(1);
      }
      can_producers.push_back(unique_ptr<Producer>(producer));
      cout << "% Created producer " << producer->name() << endl;      

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(producer, topic_name, tconf, errstr);
      if (!topic) {
        cerr << "Failed to create topic: " << errstr << endl;
        exit(1);
      } 
      kafka_can_topics.push_back(unique_ptr<Topic>(topic));    
      cout << "Producing messages into topic " << topic_name << endl;  

      ros::Subscriber can_sub = nh.subscribe<can_msgs::Frame>(can_topics[i], 1, boost::bind(can_cb, _1, i)); 

      can_subs.push_back(can_sub);
    }
  }  
  
  /*
   * nmea_msgs::Sentence
   *
   */
  if (nmea_str.size() != 0) {
    boost::split(nmea_topics, nmea_str, [](char c){return c == ',';});
    
    for (i=0; i<nmea_topics.size(); i++) {
      string ros_topic = nmea_topics[i];
      std::replace(ros_topic.begin(), ros_topic.end(), '/', '-');
      string topic_name = vehicle_id + ros_topic;    

      // Create a message builder and producer for corresponding topics  

      /*
       * Create producer using accumulated global configuration.
       */
      Producer *producer = Producer::create(conf, errstr);
      if (!producer) {
        cerr << "Failed to create producer: " << errstr << endl;
        exit(1);
      }
      nmea_producers.push_back(unique_ptr<Producer>(producer));
      cout << "% Created producer " << producer->name() << endl;      

      /*
       * Create topic handle.
       */
      Topic *topic = Topic::create(producer, topic_name, tconf, errstr);
      if (!topic) {
        cerr << "Failed to create topic: " << errstr << endl;
        exit(1);
      } 
      kafka_nmea_topics.push_back(unique_ptr<Topic>(topic));    
      cout << "Producing messages into topic " << topic_name << endl;  

      ros::Subscriber nmea_sub = nh.subscribe<nmea_msgs::Sentence>(nmea_topics[i], 1, boost::bind(nmea_cb, _1, i)); 

      nmea_subs.push_back(nmea_sub);
    }
  }
  
  
  
  ros::spin();

  return 0;
  
}  

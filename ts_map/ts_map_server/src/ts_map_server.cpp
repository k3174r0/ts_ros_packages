#include "ros/ros.h"
#include "tf/transform_datatypes.h"
#include "nav_msgs/OccupancyGrid.h"
#include "nav_msgs/GetMap.h"

std::string mapname;

class map_server
{
	public:
	ros::NodeHandle n;
	ros::Publisher map_pub, metadata_pub;
	ros::ServiceServer service;

	nav_msgs::OccupancyGrid map;
	nav_msgs::GetMap::Response map_res;

	map_server(){
		metadata_pub = n.advertise<nav_msgs::MapMetaData>("map_metadata", 1, true);
		map_pub = n.advertise<nav_msgs::OccupancyGrid>("map", 1 ,true);

		service = n.advertiseService("static_map", &map_server::mapCallback, this);

		pgm2map();
		map_pub.publish(map);
		metadata_pub.publish(map.info);
		ROS_INFO("Published Map");
	}

	bool mapCallback(nav_msgs::GetMap::Request  &req,nav_msgs::GetMap::Response &res ){
		res = map_res;
		ROS_INFO("Sending map");
		return true;
	}

	void pgm2map(){
		if(FILE* fp = fopen(mapname.c_str(), "r")){
			map.header.frame_id = "/map";
			map.data.clear();
			double x,y,yaw;
			int max;
			fscanf(fp,"P2\n");
			fscanf(fp,"#origin.x : %lf\n",&x);
			fscanf(fp,"#origin.y : %lf\n",&y);
			fscanf(fp,"#origin.yaw : %lf\n",&yaw);
			fscanf(fp,"#resolution : %f\n",&map.info.resolution);
			fscanf(fp,"%d %d\n",&map.info.width,&map.info.height);
			fscanf(fp,"%d\n",&max);

			ROS_INFO("Load Map info | origin : (%.4lf,%.4lf,%.4lf) , width : %d , height : %d , resolution : %f , max : %d",x,y,yaw,map.info.width,map.info.height,map.info.resolution,max);
			if(map.info.resolution == 0 || map.info.width == 0 || map.info.height == 0 || max == 0 || max > 255)return;
			map.info.origin.position.x = x;
			map.info.origin.position.y = y;
			map.info.origin.position.z = 0.0;
			map.info.origin.orientation = tf::createQuaternionMsgFromRollPitchYaw(0,0,yaw);
			int pixel = 0;
			int index = 0;
			map.data.clear();
			int8_t cost = -1;
			for(int i = 0;i<map.info.width*map.info.height;i++)map.data.push_back(cost);
			for(size_t row = 0;row < map.info.height;row++){
				for(size_t column = 0;column < map.info.width;column++){
					index = (map.info.height-row-1)*map.info.width+column;
					fscanf(fp,"%d ",&pixel);
					pixel *= (100/max);
					if(pixel == 50){pixel = -1;}
					else{pixel = max - pixel;}
					map.data[index] = pixel;
				}
				fscanf(fp,"\n");
			}
//			map_pub.publish(map);
//			metadata_pub.publish(map.info);
			map_res.map = map;
			fclose(fp);
		}
		else{
			ROS_ERROR("Can't open pgm file.");
			return;
		}

	}
};

int main(int argc, char** argv) {
	ros::init(argc, argv, "ts_map_server");

	if(argc == 2){
		mapname = argv[1];
		if(mapname.find(".pgm")==std::string::npos){
			ROS_ERROR("Use .pgm file");
			return 0;
		}
	}
	else{
		ROS_ERROR("Ah~ this is shit!");
		return 0;
	}
	ROS_INFO("mapfile : %s",mapname.c_str());

	map_server ms;

	ros::spin();

	return 0;
}

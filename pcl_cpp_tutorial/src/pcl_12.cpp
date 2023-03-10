#include <pcl/point_types.h>
#include <pcl/PCLPointCloud2.h>
#include <pcl/conversions.h>
#include <pcl/visualization/cloud_viewer.h>
#include <pcl/common/transforms.h>
#include <pcl/registration/gicp.h>
#include <chrono>

using namespace std;

pcl::PointCloud<pcl::PointXYZ>::ConstPtr load_bin(const string &filename){
	FILE*file=fopen(filename.c_str(), "rb");
	if(!file){
		std::cerr<<"Error : failed to load"<<filename<<std::endl;
		return nullptr;
	}
	std::vector<float> buffer(1000000);
	size_t num_points=fread(reinterpret_cast<char*>(buffer.data()), sizeof(float), buffer.size(), file)/4;
	fclose(file);
	
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>());
	cloud->resize(num_points);
	
	for(int i=0; i<num_points; i++){
		auto &pt=cloud->at(i);
		
		pt.x=buffer[i*4];
		pt.y=buffer[i*4+1];
		pt.z=buffer[i*4+2];
	}
	return cloud;
}

void colorize(const pcl::PointCloud<pcl::PointXYZ> &pc, pcl::PointCloud<pcl::PointXYZRGB> &pc_colored, const std::vector<int> &color){
	int N=pc.points.size();
	
	pc_colored.clear();
	pcl::PointXYZRGB pt_tmp;
	for(int i=0; i<N; ++i){
		const auto &pt=pc.points[i];
		
		pt_tmp.x=pt.x;
		pt_tmp.y=pt.y;
		pt_tmp.z=pt.z;
		pt_tmp.r=color[0];
		pt_tmp.g=color[1];
		pt_tmp.b=color[2];
		
		pc_colored.points.emplace_back(pt_tmp);
	}
}

int main(int argc, char **argv){
	//Load data
	pcl::PointCloud<pcl::PointXYZ>::Ptr src(new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr tgt(new pcl::PointCloud<pcl::PointXYZ>);
	*src=*load_bin("/home/maniac/velo_ws/src/pcl_cpp_tutorial/test.bin");
	
	//Test를 위해 앞으로 2m전진시킨 target을 만든다
	Eigen::Matrix4f tf;
	tf<<1, 0, 0, 2.0,
		0, 1, 0, 0.0,
		0, 0, 1, 0.0,
		0, 0, 0, 1.0;
	pcl::transformPointCloud(*src, *tgt, tf);
	
	//Main
	pcl::GeneralizedIterativeClosestPoint<pcl::PointXYZ, pcl::PointXYZ> gicp;
	gicp.setMaxCorrespondenceDistance(1.0);
	gicp.setTransformationEpsilon(0.003);
	gicp.setMaximumIterations(1000);
	
	pcl::PointCloud<pcl::PointXYZ>::Ptr align(new pcl::PointCloud<pcl::PointXYZ>);
	
	//걸리는 시간 측정
	chrono::system_clock::time_point t_start=chrono::system_clock::now();
	
	//registration 시행
	gicp.setInputSource(src);
	gicp.setInputTarget(tgt);
	gicp.align(*align);
	
	chrono::system_clock::time_point t_end=chrono::system_clock::now();
	chrono::duration<double> t_reg=t_end-t_start;
	cout<<"Takes "<<t_reg.count()<<" sec"<<endl;
	
	//Set outputs
	 Eigen::Matrix4f src2tgt   = gicp.getFinalTransformation();
    double score     = gicp.getFitnessScore();
    bool is_converged = gicp.hasConverged();
	
	cout<<"src2tgt Matrix : "<<endl;
	cout<<src2tgt<<endl;
	cout<<"Score : "<<score<<endl;
	
	pcl::PointCloud<pcl::PointXYZRGB>::Ptr src_colored(new pcl::PointCloud<pcl::PointXYZRGB>);
	pcl::PointCloud<pcl::PointXYZRGB>::Ptr tgt_colored(new pcl::PointCloud<pcl::PointXYZRGB>);
	pcl::PointCloud<pcl::PointXYZRGB>::Ptr align_colored(new pcl::PointCloud<pcl::PointXYZRGB>);
	colorize(*src, *src_colored, {255,0,0});
	colorize(*tgt, *tgt_colored, {0,255,0});
	colorize(*align, *align_colored, {0,0,255});
	
	//결과 시각화
	pcl::visualization::CloudViewer viewer("Cloud Viewer");
	viewer.showCloud(src_colored, "src_viz");
	viewer.showCloud(tgt_colored, "tgt_viz");
	viewer.showCloud(align_colored, "align_viz");
	
	int cnt=0;
	while(!viewer.wasStopped()){
		cnt++;
	}
	
	return 0;
}

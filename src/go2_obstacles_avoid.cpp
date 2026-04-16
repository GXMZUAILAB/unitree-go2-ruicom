#include <iostream>
#include <unitree/robot/go2/obstacles_avoid/obstacles_avoid_client.hpp>
#include <unitree/robot/go2/obstacles_avoid/obstacles_avoid_api.hpp>
using namespace std;
unitree::robot::ChannelFactory::Instance()->Init(0, "enx00e04c36141b");//enx00e04c36141b为网口号，用户根据自身情况修改
unitree::robot::go2::ObstaclesAvoidClient sc;
int main()
{
    sc.Init();
    sc.SetTimeout(5.0f);
    sc.SwitchSet(true);//开启避障功能
    sc.UseRemoteCommandFromApi(true);//抢夺遥控器
    if(sc.SwitchGet(true)==0)
    {
        cout<<"避障功能已开启"<<endl;
    }
    else
    {
        cout<<"避障功能开启失败"<<endl<<"错误码:"<<SwitchGet(true)<<endl;
    }
    while (true)
    {
        int ret = sc.GetObstaclesAvoidData();
        if (ret == 0) {
            auto data = sc.GetObstaclesAvoidData();
            cout << "障碍物距离: " << data.obstacle_distance << " 米" << endl;
            cout << "障碍物角度: " << data.obstacle_angle << " 度" << endl;
        }
        else {
            cout << "获取数据失败，错误码: " << ret << endl;
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    //sc.Move(1.0,0.0,0.0);//以1m的速度前进，自动避障
    sc.MoveToIncrementPosition(1.0, 0.0, 0.0);//走两步，自动避障
    // while (true)
    // {
    //   usleep(1000000);
    //   if(tem<=5)
    //   {
    //     tem++;
    //   }
    //   else
    //   {
    //     sc.UseRemoteCommandFromApi(false);//5秒后关闭
    //     sc.SwitchSet(false);//5秒后关闭
    //   }
    // }
    // 定时关闭避障功能，暂时还不知道有什么用

    return 0;
}
#include <Windows.h>
#include <Kinect.h>
#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <tchar.h>
#include <time.h>
using namespace cv;
using namespace std;


//模板类Interface
template<class Interface>

//使用模板类，用于释放体感器需要的内存空间
inline void SafeRelease( Interface *& pInterfaceToRelease )
{
    if( pInterfaceToRelease != NULL ){
        pInterfaceToRelease->Release();
        pInterfaceToRelease = NULL;
    }
}


int _tmain(int argc, _TCHAR* argv[])
{
	//计时开始
	clock_t last;
	last = clock();

    // 创建体感器实例
    IKinectSensor* pSensor;
    HRESULT hResult = S_OK;//函数返回值
    hResult = GetDefaultKinectSensor( &pSensor );
    if( FAILED( hResult ) ){
        std::cerr << "Error : GetDefaultKinectSensor" << std::endl;
        return -1;
    }

    // 打开体感器
    hResult = pSensor->Open();
    if( FAILED( hResult ) ){
        std::cerr << "Error : IKinectSensor::Open()" << std::endl;
        return -1;
    }

    // 获得坐标映射器
    ICoordinateMapper* pCoordinateMapper;
    hResult = pSensor->get_CoordinateMapper( &pCoordinateMapper );
    if( FAILED( hResult ) ){
        std::cerr << "Error : IKinectSensor::get_CoordinateMapper()" << std::endl;
        return -1;
    }

    // 获得彩色帧源
    IColorFrameSource* pColorSource;
    hResult = pSensor->get_ColorFrameSource( &pColorSource );
    if( FAILED( hResult ) ){
        std::cerr << "Error : IKinectSensor::get_ColorFrameSource()" << std::endl;
        return -1;
    }

    // 获得深度帧源
    IDepthFrameSource* pDepthSource;
    hResult = pSensor->get_DepthFrameSource( &pDepthSource );
    if( FAILED( hResult ) ){
        std::cerr << "Error : IKinectSensor::get_DepthFrameSource()" << std::endl;
        return -1;
    }

    // 打开彩色帧读出器
    IColorFrameReader* pColorReader;
    hResult = pColorSource->OpenReader( &pColorReader );
    if( FAILED( hResult ) ){
        std::cerr << "Error : IColorFrameSource::OpenReader()" << std::endl;
        return -1;
    }

    // 打开深度帧读出器
    IDepthFrameReader* pDepthReader;
    hResult = pDepthSource->OpenReader( &pDepthReader );
    if( FAILED( hResult ) ){
        std::cerr << "Error : IDepthFrameSource::OpenReader()" << std::endl;
        return -1;
    }

    // 获得彩色帧描述（大小）
    IFrameDescription* pColorDescription;
    hResult = pColorSource->get_FrameDescription( &pColorDescription );
    if( FAILED( hResult ) ){
        std::cerr << "Error : IColorFrameSource::get_FrameDescription()" << std::endl;
        return -1;
    }
    int colorWidth = 0;
    int colorHeight = 0;
    pColorDescription->get_Width( &colorWidth ); // 1920
    pColorDescription->get_Height( &colorHeight ); // 1080
    // 预留彩色帧缓冲区
    std::vector<RGBQUAD> colorBuffer( colorWidth * colorHeight );

    // 获得深度帧描述（大小）
    IFrameDescription* pDepthDescription;
    hResult = pDepthSource->get_FrameDescription( &pDepthDescription );
    if( FAILED( hResult ) ){
        std::cerr << "Error : IDepthFrameSource::get_FrameDescription()" << std::endl;
        return -1;
    }
    int depthWidth = 0;
    int depthHeight = 0;
    pDepthDescription->get_Width( &depthWidth ); // 512
    pDepthDescription->get_Height( &depthHeight ); // 424
    // 预留深度帧缓冲区
    std::vector<UINT16> depthBuffer( depthWidth * depthHeight );
	UINT nBufferSize_depth = 0;  
	UINT16 *pBuffer_depth = NULL;  

    int n = 0;
	string str;
	//计时：初始化Kinect所需时间
	Sleep(2000);
	printf("Initial The Kinect Use Time:%.1fms\n",(double(clock()-last)));
	last = clock();

	char depthFilename[20];
	char colorFilename[20];

	while(1)
	{
		if (1 == n)
			str = "st";
		else if (2 == n)
			str = "nd";
		else 
			str = "th";
		cout<<endl<<"Now Get the "<<n<<str<<" Depth And Color Data"<<endl;

		last = clock();

		Mat colorImg(depthHeight,depthWidth, CV_8UC3);
		Mat depthImg(depthHeight,depthWidth, CV_16UC1);

		// 得到最新深度帧
		IDepthFrame* pDepthFrame = nullptr;
		hResult = pDepthReader->AcquireLatestFrame( &pDepthFrame );
		if( SUCCEEDED( hResult ) )
		{
			USHORT nDepthMinReliableDistance = 0;  
			USHORT nDepthMaxReliableDistance = 0;  
			hResult = pDepthFrame->get_DepthMinReliableDistance(&nDepthMinReliableDistance);  
			hResult = pDepthFrame->get_DepthMaxReliableDistance(&nDepthMaxReliableDistance);  

			// 获得深度数据
			if (SUCCEEDED(hResult))  
			{  
				hResult = pDepthFrame->CopyFrameDataToArray( depthBuffer.size(), &depthBuffer[0] );
			}
			if (SUCCEEDED(hResult))  
			{  
				hResult = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize_depth, &pBuffer_depth);
			}
			
			// 获得深度图像
			if (SUCCEEDED(hResult))  
			{
				UINT16* p_mat = (UINT16*)depthImg.data;
				const UINT16* pBufferEnd = pBuffer_depth + (depthWidth * depthHeight); 
				while (pBuffer_depth < pBufferEnd)  
				{  
					*p_mat = *pBuffer_depth;  
					p_mat++;  
					++pBuffer_depth;  
				}

				//ColorSpacePoint *depthPoints = new ColorSpacePoint[ 512*424];
				//const UINT16 *buff = pBuffer_depth ;
				//pCoordinateMapper->MapDepthFrameToColorSpace( nBufferSize_depth, buff,512*424 , depthPoints); 

				//for (int i = 0; i < 424; i++)
				//{
				//	uchar *data = colorImg.ptr<uchar>(i);
				//	for(int j = 0; j < 512; j++)
				//	{
				//		int depthX = static_cast<int>( std::floor( depthPoints[i*512+j].X + 0.5f ) );
				//		int depthY = static_cast<int>( std::floor(  depthPoints[i*512+j].Y + 0.5f ) );
				//		cout<< i << endl << "colorX " << depthX << "   colorY " << depthY << endl;
				//		if( ( 0 <= depthX ) && ( depthX < colorWidth ) && ( 0 <= depthY ) && ( depthY < colorHeight ) )
				//		{
				//			RGBQUAD color = colorBuffer[depthY * colorWidth + depthX];
				//			data[3* j] = color.rgbBlue;
				//			data[3* j + 1] = color.rgbGreen;
				//			data[3* j + 2] = color.rgbRed;
				//		}
				//	}
				//}
				//delete depthPoints;
			}
		}
		SafeRelease( pDepthFrame );


		// 得到最新彩色帧
		IColorFrame* pColorFrame = nullptr;
		hResult = pColorReader->AcquireLatestFrame( &pColorFrame );
		if( SUCCEEDED( hResult ) )
		{
			// 获得彩色帧数据
			hResult = pColorFrame->CopyConvertedFrameDataToArray( colorBuffer.size() * sizeof( RGBQUAD ), reinterpret_cast<BYTE*>( &colorBuffer[0] ), ColorImageFormat::ColorImageFormat_Bgra );
			if( FAILED( hResult ) )
			{
				std::cerr << "Error : IColorFrame::CopyConvertedFrameDataToArray()" << std::endl;
			}

			//// 获得彩色图像
			for( int y = 0; y < depthHeight; y++ )
			{
				uchar *data = colorImg.ptr<uchar>(y);

				for( int x = 0; x < depthWidth; x++ )
				{
					DepthSpacePoint depthSpacePoint = { static_cast<float>( x ), static_cast<float>( y ) };
					UINT16 depth = depthBuffer[y * depthWidth + x];
					if(depth == 0)
					{
						depth = 7;
					}
					// 从深度空间坐标系映射到彩色空间坐标系
					ColorSpacePoint colorSpacePoint = { 0.0f, 0.0f };
					pCoordinateMapper->MapDepthPointToColorSpace( depthSpacePoint, depth, &colorSpacePoint );
					int colorX = static_cast<int>( std::floor( colorSpacePoint.X + 0.5f ) );
					int colorY = static_cast<int>( std::floor( colorSpacePoint.Y + 0.5f ) );

					if( ( 0 <= colorX ) && ( colorX < colorWidth ) && ( 0 <= colorY ) && ( colorY < colorHeight ) )
					{
						RGBQUAD color = colorBuffer[colorY * colorWidth + colorX];
						data[3 * x] = color.rgbBlue;
						data[3 * x + 1] = color.rgbGreen;
						data[3 * x + 2] = color.rgbRed;
					}
					else
					{
						//data[3 * x] = 128;
						//data[3 * x + 1] = 128;
						//data[3 * x + 2] = 128;
					}
				}
			}
		}
		SafeRelease( pColorFrame );


		// 保存图像
		sprintf_s(depthFilename,"depth/%d.png",n);
		sprintf_s(colorFilename,"rgb/%d.png",n);
		imshow("depthImg",depthImg);
		imshow("colorImg",colorImg);
		imwrite(depthFilename,depthImg);
		imwrite(colorFilename,colorImg);
		
		if (waitKey(1) == 27) break;
		

		// 计时：得到Color和Depth数据所需时间
		printf("Get The Color And Depth Data Use Time:%.1fms\n",(double(clock()-last)));
		last = clock();

		n++;
	}


	// 停止处理，释放体感器所需的内存空间
	SafeRelease( pColorSource );//彩色帧源
	SafeRelease( pDepthSource );//深度帧源
	SafeRelease( pColorReader );//彩色帧读出器
	SafeRelease( pDepthReader );//深度帧读出器
	SafeRelease( pColorDescription );//彩色帧描述
	SafeRelease( pDepthDescription );//深度帧描述
	SafeRelease( pCoordinateMapper );//坐标映射器
	//关闭体感器
	if( pSensor )
	{
		pSensor->Close();
	}
	//释放体感器
	SafeRelease( pSensor );

	return 0;
}
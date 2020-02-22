#include <stdio.h>
#include <Winsock2.h>
#include <iostream>
#include <tchar.h>
#include <time.h>
#include <Windows.h>
#include <Kinect.h>
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"

#pragma comment(lib,"WS2_32.lib")

using namespace cv;
using namespace std;

#define IMG_WIDTH 512
#define IMG_HEIGHT 424
#define COLOR_BUFFER_SIZE 81408  // 512 * 424 * 3 / 8  = 512 * 3 * 53
#define DEPTH_BUFFER_SIZE 27136  // 512 * 424 / 8  = 512 * 53 

template<class Interface>

inline void SafeRelease( Interface *& pInterfaceToRelease )
{
	if( pInterfaceToRelease != NULL ){
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

struct sentColorBuf
{
	char buf[COLOR_BUFFER_SIZE];
	int flag;
};

struct sentDepthBuf
{
	UINT16 buf[DEPTH_BUFFER_SIZE];
	int flag;
};

int main()
{
	IKinectSensor* pSensor;
	HRESULT hResult = S_OK;
	hResult = GetDefaultKinectSensor( &pSensor );
	if( FAILED( hResult ) ){
		std::cerr << "Error : GetDefaultKinectSensor" << std::endl;
		return -1;
	}

	hResult = pSensor->Open();
	if( FAILED( hResult ) ){
		std::cerr << "Error : IKinectSensor::Open()" << std::endl;
		return -1;
	}

	ICoordinateMapper* pCoordinateMapper;
	hResult = pSensor->get_CoordinateMapper( &pCoordinateMapper );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IKinectSensor::get_CoordinateMapper()" << std::endl;
		return -1;
	}

	IColorFrameSource* pColorSource;
	hResult = pSensor->get_ColorFrameSource( &pColorSource );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IKinectSensor::get_ColorFrameSource()" << std::endl;
		return -1;
	}

	IDepthFrameSource* pDepthSource;
	hResult = pSensor->get_DepthFrameSource( &pDepthSource );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IKinectSensor::get_DepthFrameSource()" << std::endl;
		return -1;
	}

	IColorFrameReader* pColorReader;
	hResult = pColorSource->OpenReader( &pColorReader );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IColorFrameSource::OpenReader()" << std::endl;
		return -1;
	}

	IDepthFrameReader* pDepthReader;
	hResult = pDepthSource->OpenReader( &pDepthReader );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IDepthFrameSource::OpenReader()" << std::endl;
		return -1;
	}

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

	std::vector<RGBQUAD> colorBuffer( colorWidth * colorHeight );

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

	std::vector<UINT16> depthBuffer( depthWidth * depthHeight );
	UINT nBufferSize_depth = 0;  
	UINT16 *pBuffer_depth = NULL;  

	Sleep(6000);

	SOCKET sockClient;
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD( 1, 1 );

	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) 
	{
		return 0;
	}
	if ( LOBYTE( wsaData.wVersion ) != 1 || HIBYTE( wsaData.wVersion ) != 1 ) 
	{
			WSACleanup( );
			return 0;
	}
	if ((sockClient = socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
		return 0;
	}
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr=inet_addr("192.168.1.103");
	addrSrv.sin_family=AF_INET;
	addrSrv.sin_port=htons(6666);
	if (connect(sockClient,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR)) < 0) 
	{
		printf("connect error: %s(errno: %d)\n", strerror(errno), errno);
		return 0;
	}

	struct sentColorBuf colorData;
	struct sentDepthBuf depthData;

	while (1)
	{
		Mat colorImg(depthHeight,depthWidth, CV_8UC3);
		Mat depthImg(depthHeight,depthWidth, CV_16UC1);

		IDepthFrame* pDepthFrame = nullptr;
		hResult = pDepthReader->AcquireLatestFrame( &pDepthFrame );
		if( SUCCEEDED( hResult ) )
		{
			USHORT nDepthMinReliableDistance = 0;  
			USHORT nDepthMaxReliableDistance = 0;  
			hResult = pDepthFrame->get_DepthMinReliableDistance(&nDepthMinReliableDistance);  
			hResult = pDepthFrame->get_DepthMaxReliableDistance(&nDepthMaxReliableDistance);  

			if (SUCCEEDED(hResult))  
			{  
				hResult = pDepthFrame->CopyFrameDataToArray( depthBuffer.size(), &depthBuffer[0] );
			}
			if (SUCCEEDED(hResult))  
			{  
				hResult = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize_depth, &pBuffer_depth);
			}
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
			}
		}
		SafeRelease( pDepthFrame );

		IColorFrame* pColorFrame = nullptr;
		hResult = pColorReader->AcquireLatestFrame( &pColorFrame );
		if( SUCCEEDED( hResult ) )
		{
			hResult = pColorFrame->CopyConvertedFrameDataToArray( colorBuffer.size() * sizeof( RGBQUAD ), reinterpret_cast<BYTE*>( &colorBuffer[0] ), ColorImageFormat::ColorImageFormat_Bgra );
			if( FAILED( hResult ) )
			{
				std::cerr << "Error : IColorFrame::CopyConvertedFrameDataToArray()" << std::endl;
			}
			for( int y = 0; y < depthHeight; y++ )
			{
				uchar *data = colorImg.ptr<uchar>(y);

				for( int x = 0; x < depthWidth; x++ )
				{
					DepthSpacePoint depthSpacePoint = { static_cast<float>( x ), static_cast<float>( y ) };
					UINT16 depth = depthBuffer[y * depthWidth + x];
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
				}
			}
		}
		SafeRelease( pColorFrame );

		for(int k = 0; k < 8; k++) 
		{
			int num1 = IMG_HEIGHT / 8 * k;
			for (int i = 0; i < IMG_HEIGHT / 8; i++)
			{
				int num2 = i * IMG_WIDTH * 3;
				uchar* ucdata = colorImg.ptr<uchar>(i + num1);
				for (int j = 0; j < IMG_WIDTH * 3; j++)
				{
					colorData.buf[num2 + j] = ucdata[j];
				}
			}

			if(k == 7)
				colorData.flag = 2;
			else
				colorData.flag = 1;

			if (send(sockClient, (char *)(&colorData), sizeof(colorData), 0) < 0)
			{
				printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
				return 0;
			}
		}

		for(int k = 0; k < 8; k++) 
		{
			int num1 = IMG_HEIGHT / 8 * k;
			for (int i = 0; i < IMG_HEIGHT / 8; i++)
			{
				int num2 = i * IMG_WIDTH;
				UINT16* usdata = depthImg.ptr<UINT16>(i + num1);
				for (int j = 0; j < IMG_WIDTH; j++)
				{
					depthData.buf[num2 + j] = usdata[j];
				}
			}
			
			if(k == 7)
				depthData.flag = 2;
			else
				depthData.flag = 1;

			if (send(sockClient, (char *)(&depthData), sizeof(depthData), 0) < 0)
			{
				printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
				return 0;
			}
		}

		imshow("",colorImg);
		waitKey(1);
	}
	closesocket(sockClient);
	WSACleanup();

	SafeRelease( pColorSource );
	SafeRelease( pDepthSource );
	SafeRelease( pColorReader );
	SafeRelease( pDepthReader );
	SafeRelease( pColorDescription );
	SafeRelease( pDepthDescription );
	SafeRelease( pCoordinateMapper );
	if( pSensor )
	{
		pSensor->Close();
	}
	SafeRelease( pSensor );

	return 0;
}
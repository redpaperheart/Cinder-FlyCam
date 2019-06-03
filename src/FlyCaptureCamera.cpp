#include "FlyCaptureCamera.h"
// Only used for standard
#define _FRAMERATE FRAMERATE_30
#define _VIDEOMODE VIDEOMODE_640x480Y8

void FlyCaptureCamera::setup(){
	mCamResX = 640 ;
	mCamResY = 480 ;
	mFps = 30;
	Error mError;

	checkIfCamIsAvailable();
	mError = mCam.SetVideoModeAndFrameRate( _VIDEOMODE, _FRAMERATE );
	
        //BRIGHTNESS, /**< Brightness. */
		//AUTO_EXPOSURE, /**< Auto exposure. */
		//SHARPNESS, /**< Sharpness */
		//WHITE_BALANCE, /**< White balance. */
		//HUE, /**< Hue. */
		//SATURATION, /**< Saturation. */
		//GAMMA, /**< Gamma. */
		//IRIS, /**< Iris. */
		//FOCUS, /**< Focus. */
		//ZOOM, /**< Zoom. */
		//PAN, /**< Pan. */
		//TILT, /**< Tilt. */
		//SHUTTER, /**< Shutter. */
		//GAIN, /**< Gain. */
		//TRIGGER_MODE, /**< Trigger mode. */
		//TRIGGER_DELAY, /**< Trigger delay. */
		//FRAME_RATE, /**< Frame rate. */
		//TEMPERATURE, /**< Temperature. */
		//UNSPECIFIED_PROPERTY_TYPE, /**< Unspecified property type. */
		//PROPERTY_TYPE_FORCE_32BITS = FULL_32BIT_VALUE
	//\a type - \c GAIN
	//* -\a absControl - \c true
	//* -\a onePush - \c false
	//* -\a onOff - \c true

	Property autoExposure;
	autoExposure.type = PropertyType::AUTO_EXPOSURE;
	autoExposure.onOff = false;
	mError = mCam.SetProperty(&autoExposure);
	
	//disable auto exposure, may also need to set the exposure or some other properties
	
	if (mError != PGRERROR_OK){
		CI_LOG_I("******************************mError setting property:");
		printError(mError);
	}
}

void FlyCaptureCamera::checkIfCamIsAvailable(){
	BusManager busMgr;
	mError = busMgr.GetNumOfCameras(&mNumCameras);
	if (mError != PGRERROR_OK){
		printError(mError);
	}
	CI_LOG_I("Attempting to connect to a FlyCapture camera");
	CI_LOG_I("Number of FlyCapture cameras detected: " << mNumCameras );
	if(mNumCameras > 0){
		CI_LOG_I("FlyCapture camera found!");
	}

	for (unsigned int i = 0; i < mNumCameras; i++){
		PGRGuid guid;
		mError = busMgr.GetCameraFromIndex(i, &guid);
		if (mError != PGRERROR_OK){
			printError(mError);
		}

		runSingleCamera(guid);
	}

	Property autoExposure;
	autoExposure.type = PropertyType::AUTO_EXPOSURE;
	autoExposure.onOff = false;
	mError = mCam.SetProperty(&autoExposure);

	//disable auto exposure, may also need to set the exposure or some other properties
	if (mError != PGRERROR_OK){
		CI_LOG_I("******************************mError setting property:");
		printError(mError);
	}
}

void FlyCaptureCamera::update(){
	if(getElapsedSeconds() < mLastFrameTime + (1.0f/mFps) ){
		return;
	}
	mLastFrameTime = getElapsedSeconds();

	// Retrieve an image
	mError = mCam.RetrieveBuffer(&mRawImage);
	if (mError != PGRERROR_OK){
		checkIfCamIsAvailable();
	}


	//mRawImage.SetDimensions(mCamResX, mCamResY, 0, PIXEL_FORMAT_RGBU, NONE);

	if (mError != PGRERROR_OK){
		printError(mError);
	}
	mError = mRawImage.Convert(PIXEL_FORMAT_RGBU, &mConvertedImage);
	if (mError != PGRERROR_OK){
		printError(mError);
	}
	
	mCamSurface = Surface::create(mCamResX, mCamResY, false, SurfaceChannelOrder::RGBA);
	mCamPixelData = mConvertedImage.GetData();
	memcpy(mCamSurface->getData(), mCamPixelData, mCamResX * mCamResY * 4);

	mCamTexture = gl::Texture::create(*mCamSurface);
	//mCamTexture->
}

void FlyCaptureCamera::draw(){
	gl::pushMatrices();
	//gl::draw( gl::Texture2d::create( mCamSurface ), mCamSurface.getBounds() );
	gl::draw( mCamTexture );
	gl::popMatrices();
}

void FlyCaptureCamera::printBuildInfo(){
	FC2Version fc2Version;
	Utilities::GetLibraryVersion(&fc2Version);

	ostringstream version;
	version << "FlyCapture2 library version: " << fc2Version.major << "."
		<< fc2Version.minor << "." << fc2Version.type << "."
		<< fc2Version.build;
	CI_LOG_I( version.str() );
	ostringstream timeStamp;
	timeStamp << "Application build date: " << __DATE__ << " " << __TIME__;
	CI_LOG_I( timeStamp.str() );
}

void FlyCaptureCamera::printError(Error error){ 
	error.PrintErrorTrace(); 
}

void FlyCaptureCamera::printCameraInfo(CameraInfo *pmCamInfo){
	CI_LOG_I( "*** CAMERA INFORMATION ***" );
	CI_LOG_I( "Serial number - " << pmCamInfo->serialNumber );
	CI_LOG_I( "Camera model - " << pmCamInfo->modelName );
	CI_LOG_I( "Camera vendor - " << pmCamInfo->vendorName );
	CI_LOG_I( "Sensor - " << pmCamInfo->sensorInfo );
	CI_LOG_I( "Resolution - " << pmCamInfo->sensorResolution );
	CI_LOG_I( "Firmware version - " << pmCamInfo->firmwareVersion );
	CI_LOG_I( "Firmware build time - " << pmCamInfo->firmwareBuildTime);
}

void FlyCaptureCamera::runSingleCamera(PGRGuid guid){
	// Connect to a camera
	mError = mCam.Connect(&guid);
	if (mError != PGRERROR_OK){
		printError(mError);
	}

	// Get the camera information
	mError = mCam.GetCameraInfo(&mCamInfo);
	if (mError != PGRERROR_OK){
		printError(mError);
	}

	printCameraInfo(&mCamInfo);

	// Get the camera configuration
	FC2Config config;
	mError = mCam.GetConfiguration(&config);
	if (mError != PGRERROR_OK){
		printError(mError);
	}

	// Set the number of driver buffers used to 10.
	config.numBuffers = 10;

	// Set the camera configuration
	mError = mCam.SetConfiguration(&config);
	if (mError != PGRERROR_OK){
		printError(mError);
	}

	startCamera();
}
/*
void FlyCaptureCamera::saveFramesToDisc(int numFrames){
	for (int imageCnt = 0; imageCnt < numFrames; imageCnt++){
		// Retrieve an image
		mError = mCam.RetrieveBuffer(&mRawImage);
		if (mError != PGRERROR_OK){
			printError(mError);
			continue;
		}

		cout << "Grabbed image " << imageCnt << endl;

		// Convert the raw image
		mError = mRawImage.Convert(PIXEL_FORMAT_MONO8, &mConvertedImage);
		if (mError != PGRERROR_OK){
			printError(mError);
		}

		// Create a unique filename

		ostringstream filename;
		filename << "FlyCapture2Test-" << mCamInfo.serialNumber << "-"
			<< imageCnt << ".png"; // could also be .pgm
		mError = mConvertedImage.Save(filename.str().c_str());

		CI_LOG_I( "total data size: " << mConvertedImage.GetDataSize() );
		if (mError != PGRERROR_OK){
			printError(mError);
		}
	}
}
*/

void FlyCaptureCamera::startCamera(){
	// Start capturing images
	mError = mCam.StartCapture();
	if (mError != PGRERROR_OK){
		printError(mError);
	}
}


void FlyCaptureCamera::stopCamera(){
	// Stop capturing images
	mError = mCam.StopCapture();
	if (mError != PGRERROR_OK){
		printError(mError);
	}

	// Disconnect the camera
	mError = mCam.Disconnect();
	if (mError != PGRERROR_OK){
		printError(mError);
	}
}

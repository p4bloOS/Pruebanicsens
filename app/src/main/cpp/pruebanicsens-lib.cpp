/*
 * Utility library to access the camera natively and get the data required by the "pruebanicsens"
 * application, using the camera2ndk library.
 */

#include <jni.h>
#include <string>
#include <pthread.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraManager.h>
#include <media/NdkImage.h>
#include <vector>
#include <media/NdkImageReader.h>
#include <fstream>
#include <iostream>

// Return codes
#define SUCCESS 0
#define ERROR -1

// Logging tag
const char * TAG = "pruebanicsens-lib";

// Camera objects
static ANativeWindow *theNativeWindow;
static ACameraDevice *cameraDevice;
static ACaptureRequest *captureRequest; // <- request for continuous preview
static ACaptureRequest *captureImageRequest; // <- request for individual photo
static ACameraOutputTarget *cameraOutputTarget;
static ACaptureSessionOutput *sessionOutput;
static ACaptureSessionOutputContainer *captureSessionOutputContainer;
static ACameraCaptureSession *captureSession;
static ACameraDevice_StateCallbacks deviceStateCallbacks;

static bool isCaptureImageInitialized = false;
static std::string imageFileName;
static std::pair<int,int> maximumResolution;

// CAPTURE SESSION CALLBACKS
static ACameraCaptureSession_stateCallbacks captureSessionStateCallbacks;
static void camera_device_on_disconnected(void *context, ACameraDevice *device) {
    __android_log_print(ANDROID_LOG_DEBUG,TAG,"Camera(id: %s) is diconnected.\n", ACameraDevice_getId(device));
}
static void camera_device_on_error(void *context, ACameraDevice *device, int error) {
    __android_log_print(ANDROID_LOG_ERROR,TAG,"Error(code: %d) on Camera(id: %s).\n", error, ACameraDevice_getId(device));
}
static void capture_session_on_ready(void *context, ACameraCaptureSession *session) {
    __android_log_print(ANDROID_LOG_DEBUG,TAG,"Session is ready. %p\n", session);
}
static void capture_session_on_active(void *context, ACameraCaptureSession *session) {
    __android_log_print(ANDROID_LOG_DEBUG,TAG,"Session is activated. %p\n", session);
}
static void capture_session_on_closed(void *context, ACameraCaptureSession *session) {
    __android_log_print(ANDROID_LOG_DEBUG,TAG,"Session is closed. %p\n", session);
}

// CAPTURE CALLBAKS (FOR INDIVIDUAL CAPTURE)
static ACameraCaptureSession_captureCallbacks captureCallbacks;
static void onCaptureFailed(void* context, ACameraCaptureSession* session,
                     ACaptureRequest* request, ACameraCaptureFailure* failure)
{
    __android_log_print(ANDROID_LOG_ERROR,TAG,"Individual capture has failed");
}
static void onCaptureCompleted (
        void* context, ACameraCaptureSession* session,
        ACaptureRequest* request, const ACameraMetadata* result)
{
    __android_log_print(ANDROID_LOG_DEBUG,TAG,"Individual capture has completed successfully");
}
static AImageReader_ImageListener imageListener;
static void onImageAvailable(void *context, AImageReader * reader) {
    AImage *image = nullptr;
    // Adquirir la siguiente imagen disponible
    if (AImageReader_acquireNextImage(reader, &image) != AMEDIA_OK) {
        __android_log_print(ANDROID_LOG_ERROR,TAG,"Error ocurred while acquiring the image");
        return;
    }
    int32_t format;
    AImage_getFormat(image, &format);
    if (format == AIMAGE_FORMAT_YUV_420_888) {
        __android_log_print(ANDROID_LOG_DEBUG,TAG,"Image is YUV_420 format");
    }

    int32_t width, height;
    AImage_getWidth(image, &width);
    AImage_getHeight(image, &height);
    __android_log_print(ANDROID_LOG_DEBUG,TAG,"Image resolution: %dx%d", width, height);

    uint8_t *yData = nullptr;
    uint8_t *uData = nullptr;
    uint8_t *vData = nullptr;
    int32_t yStride, uStride, vStride;

    AImage_getPlaneData(image, 0, &yData, &yStride); // Y plane
    AImage_getPlaneData(image, 1, &uData, &uStride); // U plane
    AImage_getPlaneData(image, 2, &vData, &vStride); // V plane

    // Guardar los datos en un archivo
    __android_log_print(ANDROID_LOG_INFO, TAG, "Saving image in %s...", imageFileName.c_str());
    std::ofstream outFile(imageFileName, std::ios::binary);
    if (outFile.is_open()) {
        // Escribir el plano Y
        outFile.write(reinterpret_cast<const char*>(yData), width * height);
        // Escribir el plano U
        outFile.write(reinterpret_cast<const char*>(uData), (width / 2) * (height / 2));
        // Escribir el plano V
        outFile.write(reinterpret_cast<const char*>(vData), (width / 2) * (height / 2));
        outFile.close();
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "YUV420 saved.");
    } else {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Error ocurred while opening file to store the image.");
    }
    AImage_delete(image);
}


std::pair<int,int> getMaxResolution(ACameraMetadata *cameraMetadata) {
    __android_log_print(ANDROID_LOG_INFO,TAG,"Obtaining max. resolution...");
    std::pair<int,int> maxResolution(0,0);
    ACameraMetadata_const_entry resolutions_entry = {0};

    ACameraMetadata_getConstEntry(cameraMetadata, acamera_metadata_tag::ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &resolutions_entry);
    for (int i = 0; i < resolutions_entry.count; i += 4) {

        // Ignore input stream
        int32_t input = resolutions_entry.data.i32[i+3];
        if (input) {
            continue;
        }

        // Output stream
        int32_t format = resolutions_entry.data.i32[i+0];
        if (format == AIMAGE_FORMAT_YUV_420_888)
        {
            int32_t width = resolutions_entry.data.i32[i+1];
            int32_t height = resolutions_entry.data.i32[i+2];
            if (width*height > maxResolution.first * maxResolution.second) {
                maxResolution.first = width;
                maxResolution.second = height;
            }
        }
    }
    __android_log_print(ANDROID_LOG_INFO,TAG,"Max resolution is %dx%d", maxResolution.first, maxResolution.second);
    return maxResolution;
}



static int openCamera(ACameraDevice_request_template templateId)
{
    __android_log_print(ANDROID_LOG_INFO,TAG,"Opening camera...");
    ACameraIdList *cameraIdList = NULL;
    ACameraMetadata *cameraMetadata = NULL;

    const char *selectedCameraId = NULL;
    camera_status_t camera_status = ACAMERA_OK;
    ACameraManager *cameraManager = ACameraManager_create();

    camera_status = ACameraManager_getCameraIdList(cameraManager, &cameraIdList);
    if (camera_status != ACAMERA_OK) {
        __android_log_print(ANDROID_LOG_ERROR,TAG,"Failed to get camera id list (reason: %d)\n", camera_status);
        return ERROR;
    }

    if (cameraIdList->numCameras < 1) {
        __android_log_print(ANDROID_LOG_ERROR,TAG,"No camera device detected.\n");
        return ERROR;
    }

    selectedCameraId = cameraIdList->cameraIds[0];

    __android_log_print(ANDROID_LOG_DEBUG,"Trying to open Camera2 (id: %s, num of camera : %d)\n", selectedCameraId,
         cameraIdList->numCameras);

    camera_status = ACameraManager_getCameraCharacteristics(cameraManager, selectedCameraId,
                                                            &cameraMetadata);

    maximumResolution = getMaxResolution(cameraMetadata);

    if (camera_status != ACAMERA_OK) {
        __android_log_print(ANDROID_LOG_ERROR,TAG,"Failed to get camera meta data of ID:%s\n", selectedCameraId);
        return ERROR;
    }

    deviceStateCallbacks.onDisconnected = camera_device_on_disconnected;
    deviceStateCallbacks.onError = camera_device_on_error;

    camera_status = ACameraManager_openCamera(cameraManager, selectedCameraId,
                                              &deviceStateCallbacks, &cameraDevice);

    if (camera_status != ACAMERA_OK) {
        __android_log_print(ANDROID_LOG_ERROR,TAG,"Failed to open camera device (id: %s)\n", selectedCameraId);
    }

    camera_status = ACameraDevice_createCaptureRequest(cameraDevice, templateId,
                                                       &captureRequest);

    if (camera_status != ACAMERA_OK) {
        __android_log_print(ANDROID_LOG_ERROR,TAG,"Failed to create preview capture request (id: %s)\n", selectedCameraId);
        return ERROR;
    }

    ACaptureSessionOutputContainer_create(&captureSessionOutputContainer);

    // Set callbacks
    captureSessionStateCallbacks.onReady = capture_session_on_ready;
    captureSessionStateCallbacks.onActive = capture_session_on_active;
    captureSessionStateCallbacks.onClosed = capture_session_on_closed;
    captureCallbacks.onCaptureCompleted = onCaptureCompleted;
    captureCallbacks.onCaptureFailed = onCaptureFailed;
    imageListener.onImageAvailable = onImageAvailable;

    ACameraMetadata_free(cameraMetadata);
    ACameraManager_deleteCameraIdList(cameraIdList);
    ACameraManager_delete(cameraManager);

    return SUCCESS;
}


static int closeCamera(void)
{
    camera_status_t camera_status = ACAMERA_OK;

    if (captureRequest != NULL) {
        ACaptureRequest_free(captureRequest);
        captureRequest = NULL;
    }

    if (captureImageRequest != NULL) {
        ACaptureRequest_free(captureImageRequest);
        captureImageRequest = NULL;
    }
    isCaptureImageInitialized = false;

    if (cameraOutputTarget != NULL) {
        ACameraOutputTarget_free(cameraOutputTarget);
        cameraOutputTarget = NULL;
    }

    if (cameraDevice != NULL) {
        camera_status = ACameraDevice_close(cameraDevice);

        if (camera_status != ACAMERA_OK) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to close CameraDevice.\n");
            return ERROR;
        }
        cameraDevice = NULL;
    }

    if (sessionOutput != NULL) {
        ACaptureSessionOutput_free(sessionOutput);
        sessionOutput = NULL;
    }

    if (captureSessionOutputContainer != NULL) {
        ACaptureSessionOutputContainer_free(captureSessionOutputContainer);
        captureSessionOutputContainer = NULL;
    }

    __android_log_print(ANDROID_LOG_INFO, TAG,"Camera is closed\n");
    return SUCCESS;
}





extern "C"
JNIEXPORT int JNICALL
Java_app_pgiherman_pruebanicsens_camera_CameraControl_jniStartPreview(JNIEnv *env, jobject thiz, jobject surface) {

    theNativeWindow = ANativeWindow_fromSurface(env, surface);

    if (openCamera(TEMPLATE_PREVIEW) != SUCCESS) {return ERROR;};

    __android_log_print(ANDROID_LOG_DEBUG,TAG,"Surface is prepared in %p.\n", surface);

    ACameraOutputTarget_create(theNativeWindow, &cameraOutputTarget);
    ACaptureRequest_addTarget(captureRequest, cameraOutputTarget);

    ACaptureSessionOutput_create(theNativeWindow, &sessionOutput);
    ACaptureSessionOutputContainer_add(captureSessionOutputContainer, sessionOutput);

    ACameraDevice_createCaptureSession(cameraDevice, captureSessionOutputContainer,
                                       &captureSessionStateCallbacks, &captureSession);

    ACameraCaptureSession_setRepeatingRequest(captureSession, NULL, 1, &captureRequest, NULL);

    return SUCCESS;

}

extern "C"
JNIEXPORT int JNICALL
Java_app_pgiherman_pruebanicsens_camera_CameraControl_jniStopPreview(JNIEnv *env, jobject thiz) {

    if (closeCamera() != SUCCESS) {return ERROR;};
    if (theNativeWindow != NULL) {
        ANativeWindow_release(theNativeWindow);
        theNativeWindow = NULL;
    }
    return SUCCESS;

}


/*
 * Returns an array with all available resolutions. The even positions store the width, and their
 * consecutive odd postions store the height. For example: {1024, 768, 1920, 1080} represents the
 * resolutions 1024x768 and 1920x1080.
 */
extern "C"
JNIEXPORT jintArray JNICALL
Java_app_pgiherman_pruebanicsens_camera_CameraControl_jniGetResolutions(JNIEnv *env, jobject thiz) {
    __android_log_print(ANDROID_LOG_INFO,TAG,"Obtaining resolutions...");

    ACameraMetadata *cameraMetadata = NULL;
    ACameraIdList *cameraIdList = NULL;
    const char *selectedCameraId = NULL;
    camera_status_t camera_status = ACAMERA_OK;
    ACameraManager *cameraManager = ACameraManager_create();
    ACameraMetadata_const_entry resolutions_entry = {0};
    jclass exceptionClass = env->FindClass("java/lang/RuntimeException");
    std::vector<int> resolutionsVector {};

    camera_status = ACameraManager_getCameraIdList(cameraManager, &cameraIdList);
    if (camera_status != ACAMERA_OK) {
        env->ThrowNew(exceptionClass, "Failed to get camera id list");
    }

    if (cameraIdList->numCameras < 1) {
        const char *  errorMsg = "No camera device detected.\n";
        env->ThrowNew(exceptionClass, errorMsg);
    }

    selectedCameraId = cameraIdList->cameraIds[0];

    __android_log_print(ANDROID_LOG_INFO,TAG,"Trying to get resolutions (id: %s, num of camera : %d)\n", selectedCameraId,
                        cameraIdList->numCameras);

    camera_status = ACameraManager_getCameraCharacteristics(cameraManager, selectedCameraId,
                                                            &cameraMetadata);

    if (camera_status != ACAMERA_OK) {
        std::string errorMsg = "Failed to get camera meta data of ID: " + std::string(selectedCameraId) + "\n";
        env->ThrowNew(exceptionClass, errorMsg.c_str());
    }


    ACameraMetadata_getConstEntry(cameraMetadata, acamera_metadata_tag::ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &resolutions_entry);
    for (int i = 0; i < resolutions_entry.count; i += 4) {

        // Ignore input stream
        int32_t input = resolutions_entry.data.i32[i+3];
        if (input) {
            continue;
        }

        // Output stream
        int32_t format = resolutions_entry.data.i32[i+0];
        if (format == AIMAGE_FORMAT_YUV_420_888)
        {
            int32_t width = resolutions_entry.data.i32[i+1];
            int32_t height = resolutions_entry.data.i32[i+2];
            resolutionsVector.push_back(width);
            resolutionsVector.push_back(height);
            // __android_log_print(ANDROID_LOG_VERBOSE,TAG,"RESOLUCION_YUV_420: %" PRId32 "x%" PRId32 "\n", width, height);
        }

    }

    ACameraMetadata_free(cameraMetadata);
    ACameraManager_deleteCameraIdList(cameraIdList);
    ACameraManager_delete(cameraManager);

    jintArray resolutionsJNIArray = env->NewIntArray(resolutionsVector.size());
    env->SetIntArrayRegion(resolutionsJNIArray, 0, resolutionsVector.size(), resolutionsVector.data());
    return resolutionsJNIArray;
}


extern "C"
JNIEXPORT void JNICALL
Java_app_pgiherman_pruebanicsens_camera_CameraControl_jniCaptureImage(JNIEnv *env, jobject thiz, jstring fileName) {
    const char * strFileName = env->GetStringUTFChars(fileName, nullptr);
    imageFileName = std::string(strFileName);
    __android_log_print(ANDROID_LOG_INFO,TAG,"Capturing image... %s", strFileName);

    static ACaptureRequest * imageCaptureRequest;

    if (!isCaptureImageInitialized) {

        // Assuming that camera preview has already been started
        ACameraCaptureSession_stopRepeating(captureSession);

        // Image Reader
        AImageReader * imageReader = nullptr;
        media_status_t mediaStatus;
        int width = maximumResolution.first;
        int height = maximumResolution.second;
        int maxImages = 1;
        mediaStatus = AImageReader_new(width, height, AIMAGE_FORMAT_YUV_420_888, maxImages, &imageReader);
        if (mediaStatus != media_status_t::AMEDIA_OK) {
        __android_log_print(ANDROID_LOG_ERROR,TAG,"Error ocurred while creating the AImageReader");
        return;
        }
        AImageReader_setImageListener(imageReader, &imageListener);

        // Image Reader -->> Window
        static ANativeWindow * imageWindow;
        AImageReader_getWindow(imageReader, &imageWindow);

        // Window -->> Output target
        static ACameraOutputTarget *imageOutputTarget;
        ACameraOutputTarget_create(imageWindow, &imageOutputTarget);

        // Output target -->> CaptureRequest
        ACameraDevice_createCaptureRequest(cameraDevice, TEMPLATE_STILL_CAPTURE,
                &imageCaptureRequest);
        ACaptureRequest_addTarget(imageCaptureRequest, imageOutputTarget);

        // Window -->> CaptureSessionOutput -->> CaptureSessionOutputContainer
        static ACaptureSessionOutput *imageSessionOutput;
        ACaptureSessionOutput_create(imageWindow, &imageSessionOutput);
        ACaptureSessionOutputContainer_add(captureSessionOutputContainer,
                imageSessionOutput);

        // The session must be recreated to have the updated CaptureSessionOutputContainer
        ACameraCaptureSession_close(captureSession);
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "Recreating capture session");
        ACameraDevice_createCaptureSession(cameraDevice, captureSessionOutputContainer,
                &captureSessionStateCallbacks, &captureSession);
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "Recreated capture session");

        // Preview
        ACameraCaptureSession_setRepeatingRequest(captureSession, NULL, 1, &captureRequest, NULL);

        isCaptureImageInitialized = true;
    }

    ACameraCaptureSession_capture(captureSession, &captureCallbacks, 1, &imageCaptureRequest, NULL);

}
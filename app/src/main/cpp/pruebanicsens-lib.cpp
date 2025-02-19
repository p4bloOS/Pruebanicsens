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

// Return codes
#define SUCCESS 0
#define ERROR -1

// Logging tag
const char * TAG = "pruebanicsens-lib";

// Camera objects
static ANativeWindow *theNativeWindow;
static ACameraDevice *cameraDevice;
static ACaptureRequest *captureRequest;
static ACameraOutputTarget *cameraOutputTarget;
static ACaptureSessionOutput *sessionOutput;
static ACaptureSessionOutputContainer *captureSessionOutputContainer;
static ACameraCaptureSession *captureSession;
static ACameraDevice_StateCallbacks deviceStateCallbacks;
static ACameraCaptureSession_stateCallbacks captureSessionStateCallbacks;

// CALLBACKS
static void camera_device_on_disconnected(void *context, ACameraDevice *device) {
    __android_log_print(ANDROID_LOG_INFO,TAG,"Camera(id: %s) is diconnected.\n", ACameraDevice_getId(device));
}
static void camera_device_on_error(void *context, ACameraDevice *device, int error) {
    __android_log_print(ANDROID_LOG_ERROR,TAG,"Error(code: %d) on Camera(id: %s).\n", error, ACameraDevice_getId(device));
}
static void capture_session_on_ready(void *context, ACameraCaptureSession *session) {
    __android_log_print(ANDROID_LOG_INFO,TAG,"Session is ready. %p\n", session);
}
static void capture_session_on_active(void *context, ACameraCaptureSession *session) {
    __android_log_print(ANDROID_LOG_INFO,TAG,"Session is activated. %p\n", session);
}
static void capture_session_on_closed(void *context, ACameraCaptureSession *session) {
    __android_log_print(ANDROID_LOG_INFO,TAG,"Session is closed. %p\n", session);
}


extern "C" JNIEXPORT jstring JNICALL
Java_app_pgiherman_pruebanicsens_camera_CameraControl_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}


static int openCamera(ACameraDevice_request_template templateId)
{
    __android_log_print(ANDROID_LOG_WARN,TAG,"Opening camera...");
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

    __android_log_print(ANDROID_LOG_INFO,TAG,"Trying to open Camera2 (id: %s, num of camera : %d)\n", selectedCameraId,
         cameraIdList->numCameras);

    camera_status = ACameraManager_getCameraCharacteristics(cameraManager, selectedCameraId,
                                                            &cameraMetadata);

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

    captureSessionStateCallbacks.onReady = capture_session_on_ready;
    captureSessionStateCallbacks.onActive = capture_session_on_active;
    captureSessionStateCallbacks.onClosed = capture_session_on_closed;

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

    __android_log_print(ANDROID_LOG_INFO, TAG,"Close Camera\n");
    return SUCCESS;
}


extern "C"
JNIEXPORT int JNICALL
Java_app_pgiherman_pruebanicsens_camera_CameraControl_jniStartPreview(JNIEnv *env, jobject thiz, jobject surface) {

    theNativeWindow = ANativeWindow_fromSurface(env, surface);

    if (openCamera(TEMPLATE_PREVIEW) != SUCCESS) {return ERROR;};

    __android_log_print(ANDROID_LOG_INFO,TAG,"Surface is prepared in %p.\n", surface);

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
package com.cuifei.testffmpeg;

import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Toast;
import java.io.IOException;
import java.lang.ref.WeakReference;

/**
 * Created by cuifei on 2017/3/11.
 */

public class LiveActivity extends AppCompatActivity implements SurfaceHolder.Callback ,Camera.PreviewCallback{

  public static final int STREAMER_INIT =0;
  public static final int STREAMER_HANDLE =1;
  public static final int STREAMER_RELEASE =2;
  public static final int STREAMER_FLUSH =3;

  long startTime;
  SurfaceHolder surfaceHolder;
  SurfaceView surfaceView;
  Camera camera;

  HandlerThread handlerThread;
  LiveHandler liveHandler;

  private int liveInitResult = -1;

  int width = 720;
  int height = 480;

  @Override protected void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_live);
    surfaceView = (SurfaceView) findViewById(R.id.surfaceView);
    System.out.print("===onCreate=="+liveInitResult);
    surfaceHolder = surfaceView.getHolder();
    surfaceHolder.setFixedSize(width,height);
    surfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
    surfaceHolder.addCallback(this);

    handlerThread = new HandlerThread("liveHandlerThread");
    handlerThread.start();
    liveHandler = new LiveHandler(this,handlerThread.getLooper());
  }

  @Override public void surfaceCreated(SurfaceHolder surfaceHolder) {
    liveInitResult = streamerInit(width,height);
    Toast.makeText(this,""+liveInitResult,Toast.LENGTH_LONG).show();
    System.out.print("===liveInitResult=="+liveInitResult);
    if (liveInitResult == -1){
      liveHandler.sendEmptyMessage(STREAMER_RELEASE);
    }

    try {
      camera = Camera.open(0);
      camera.setPreviewDisplay(surfaceHolder);
      Camera.Parameters params = camera.getParameters();
      //设置预览大小
      params.setPreviewSize(width, height);
      //设置生成的照片大小
      params.setPictureSize(width, height);

      int bufferSize = width * height * 3 /2;
      params.setPreviewFormat(ImageFormat.NV21);
      camera.setDisplayOrientation(90);
      camera.setParameters(params);
      camera.addCallbackBuffer(new byte[bufferSize]);
      camera.addCallbackBuffer(new byte[bufferSize]);
      camera.setPreviewCallbackWithBuffer(this);
      //camera.setPreviewCallback(this);
      startTime = System.currentTimeMillis();
      camera.startPreview();
    } catch (IOException e) {
      e.printStackTrace();
    }
  }

  @Override public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {

  }

  @Override public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
    if(liveInitResult == 0) {
      liveHandler.sendEmptyMessage(STREAMER_FLUSH);
    }

    liveHandler.sendEmptyMessage(STREAMER_RELEASE);
    handlerThread.quitSafely();
    destroyCamera();
  }

  @Override protected void onDestroy() {
    super.onDestroy();
  }

  private void destroyCamera() {
    if(camera == null) {
      return;
    }

    camera.setPreviewCallback(null);
    camera.stopPreview();
    camera.release();
    camera = null;
  }

  @Override public void onPreviewFrame(byte[] bytes, Camera camera) {
    System.out.print("====="+bytes.length);

    long timetamp = 1000 * (System.currentTimeMillis() - startTime);

    if (liveInitResult == 0 && bytes != null && bytes.length >0){
      Message message = Message.obtain();
      message.what = STREAMER_HANDLE;
      Bundle bundle = new Bundle();
      bundle.putByteArray("frame_data",bytes);
      bundle.putLong("timetamp",timetamp);
      message.setData(bundle);
      liveHandler.handleMessage(message);
    }

    camera.addCallbackBuffer(bytes);
  }

  private native int streamerInit(int width,int height);

  private native int streamerHandle(byte[] data,long timetamp);

  private native int streamerFlush();

  private native int streamRelease();


  private static class LiveHandler extends Handler{
    private WeakReference<LiveActivity> mWeakReference;
    public LiveHandler(LiveActivity liveActivity,Looper looper){
      super(looper);
      this.mWeakReference = new WeakReference<LiveActivity>(liveActivity);
    }
    @Override public void handleMessage(Message msg) {
      super.handleMessage(msg);

      LiveActivity liveActivity = mWeakReference.get();
      if (liveActivity == null){
        return;
      }

      switch (msg.what){
        case STREAMER_INIT:

          break;
        case STREAMER_HANDLE:
            Bundle bundle = (Bundle) msg.getData();
          if (bundle != null){
            byte[] data = bundle.getByteArray("frame_data");
            long timetamp = bundle.getLong("timetamp");
            if (data != null && data.length > 0){
              liveActivity.streamerHandle(data,timetamp);
            }else{
              Log.e("==LiveActivity", "byte data null");
            }
          }else {
            Log.e("==LiveActivity", "bundle null");
          }

          break;
        case STREAMER_FLUSH:
          liveActivity.streamerFlush();
          break;
        case STREAMER_RELEASE:
          liveActivity.streamRelease();
          break;
      }


    }
  }
}





















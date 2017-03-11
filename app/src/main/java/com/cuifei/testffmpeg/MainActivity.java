package com.cuifei.testffmpeg;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

  @Override protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    TextView textView = (TextView) findViewById(R.id.test);
    textView.setText(helloFFmpeg());
  }

   public native String helloFFmpeg();

  static {
    System.loadLibrary("live_jni");
  }
}

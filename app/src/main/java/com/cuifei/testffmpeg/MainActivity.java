package com.cuifei.testffmpeg;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

  @Override protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    //TextView textView = (TextView) findViewById(R.id.test);
    //textView.setText(helloFFmpeg());
  }

   public native String helloFFmpeg();

  public void onClick(View view){
    switch (view.getId()){
      case R.id.live:
        Intent intent = new Intent(this,LiveActivity.class);
        startActivity(intent);
        break;
      case R.id.look:

        break;

    }
  }

  static {
    System.loadLibrary("live_jni");
  }
}

package com.hanto.client;
import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.Toast;
import java.io.DataOutputStream;
import java.io.BufferedReader;
import java.io.InputStreamReader;

public class MainActivity extends Activity {
    static { System.loadLibrary("hantoo"); }
    public native void nativeInject(int pid, String libPath);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(60, 60, 60, 60);
        Button startBtn = new Button(this);
        startBtn.setText("START STANDOFF 2 + INJECT");
        startBtn.setTextSize(18f);
        startBtn.setOnClickListener(v -> {
            if (!Settings.canDrawOverlays(this)) {
                startActivity(new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + getPackageName())));
                return;
            }
            startBtn.setText("Injecting...");
            startBtn.setEnabled(false);
            new Thread(() -> {
                try {
                    Process p = Runtime.getRuntime().exec("su");
                    DataOutputStream os = new DataOutputStream(p.getOutputStream());
                    os.writeBytes("killall com.axlebolt.standoff2\n");
                    os.writeBytes("sleep 1\n");
                    os.writeBytes("monkey -p com.axlebolt.standoff2 -c android.intent.category.LAUNCHER 1\n");
                    os.writeBytes("exit\n");
                    os.flush();
                    Thread.sleep(12000);
                    Process pidProc = Runtime.getRuntime().exec("su -c pidof com.axlebolt.standoff2");
                    BufferedReader reader = new BufferedReader(new InputStreamReader(pidProc.getInputStream()));
                    String pidStr = reader.readLine();
                    if (pidStr != null && !pidStr.isEmpty()) {
                        int pid = Integer.parseInt(pidStr.trim().split(" ")[0]);
                        nativeInject(pid, getApplicationInfo().nativeLibraryDir + "/libhantoo.so");
                        runOnUiThread(() -> {
                            Toast.makeText(MainActivity.this, "Injected!", Toast.LENGTH_LONG).show();
                            startService(new Intent(MainActivity.this, OverlayService.class));
                            startBtn.setText("START");
                            startBtn.setEnabled(true);
                        });
                    }
                } catch (Exception e) {
                    runOnUiThread(() -> { startBtn.setText("ERROR"); startBtn.setEnabled(true); });
                }
            }).start();
        });
        layout.addView(startBtn);
        setContentView(layout);
    }
}

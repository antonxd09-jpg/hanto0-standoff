package com.hanto0.standoff;
import android.app.Activity;
import android.os.Bundle;
import android.widget.Button;
import android.widget.Toast;
import java.io.*;

public class MainActivity extends Activity {
    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        Button btn = new Button(this);
        btn.setText("START");
        setContentView(btn);
        btn.setOnClickListener(v -> {
            Toast.makeText(this, "Launching...", Toast.LENGTH_SHORT).show();
            new Thread(() -> runRoot()).start();
        });
    }
    
    private void copyAsset(String name, File dest) throws Exception {
        InputStream is = getAssets().open(name);
        FileOutputStream fos = new FileOutputStream(dest);
        byte[] buf = new byte[8192]; int n;
        while ((n = is.read(buf)) > 0) fos.write(buf, 0, n);
        fos.close(); is.close();
        dest.setExecutable(true);
    }

    private void runRoot() {
        try {
            File outSo = new File(getFilesDir(), "libnl_menu.so");
            File outInjector = new File(getFilesDir(), "injector");
            
            if (!outSo.exists()) copyAsset("libnl_menu.so", outSo);
            if (!outInjector.exists()) copyAsset("injector", outInjector);

            Process p = Runtime.getRuntime().exec("su");
            DataOutputStream os = new DataOutputStream(p.getOutputStream());
            
            os.writeBytes("cp " + outSo.getAbsolutePath() + " /data/local/tmp/\n");
            os.writeBytes("cp " + outInjector.getAbsolutePath() + " /data/local/tmp/\n");
            os.writeBytes("chmod 755 /data/local/tmp/*\n");
            
            os.writeBytes("am start -n com.axlebolt.standoff2/com.axlebolt.standoff2.MainActivity\n");
            os.writeBytes("sleep 10\n");
            
            os.writeBytes("PID=$(pidof com.axlebolt.standoff2)\n");
            os.writeBytes("/data/local/tmp/injector $PID /data/local/tmp/libnl_menu.so\n");
            
            os.writeBytes("exit\n");
            os.flush(); p.waitFor();
            
            runOnUiThread(() -> Toast.makeText(this, "Injected", Toast.LENGTH_SHORT).show());
        } catch (Exception e) {
            runOnUiThread(() -> Toast.makeText(this, "Error: " + e.getMessage(), Toast.LENGTH_LONG).show());
        }
    }
}

package com.hanto0.standoff;
import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.widget.*;
import java.io.*;

public class MainActivity extends Activity {
    private TextView status;
    private Button startBtn;
    private DataOutputStream out;
    private BufferedReader in;
    
    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        ScrollView scroll = new ScrollView(this);
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(40, 40, 40, 40);
        
        status = new TextView(this);
        status.setTextSize(12);
        status.setText("=== Hanto0 Menu Launcher ===\nTap START to begin.\n\n");
        
        startBtn = new Button(this);
        startBtn.setText("START");
        startBtn.setTextSize(22);
        startBtn.setOnClickListener(v -> {
            startBtn.setEnabled(false);
            new Thread(() -> run()).start();
        });
        
        layout.addView(startBtn);
        layout.addView(status);
        scroll.addView(layout);
        setContentView(scroll);
    }
    
    private void log(String msg) {
        Log.d("Hanto0", msg);
        runOnUiThread(() -> status.append(msg + "\n"));
    }
    
    private void run() {
        try {
            log("[*] Checking root...");
            Process p = Runtime.getRuntime().exec("su");
            out = new DataOutputStream(p.getOutputStream());
            in = new BufferedReader(new InputStreamReader(p.getInputStream()));
            
            String id = cmd("id");
            log("> " + id);
            if (id == null || !id.contains("uid=0")) { log("[!] No root!"); return; }
            
            log("[*] Disabling security...");
            cmd("setenforce 0 2>/dev/null; echo 0 > /proc/sys/kernel/yama/ptrace_scope 2>/dev/null; echo done");
            
            log("[*] Copying files...");
            File so = new File(getFilesDir(), "libnl_menu.so");
            File inj = new File(getFilesDir(), "injector");
            
            // ВИПРАВЛЕНО: Видаляємо старі файли, щоб оновити на нові
            if (so.exists()) so.delete();
            if (inj.exists()) inj.delete();
            
            copyAsset("libnl_menu.so", so);
            copyAsset("injector", inj);
            
            cmd("cp " + so.getAbsolutePath() + " /data/local/tmp/");
            cmd("cp " + inj.getAbsolutePath() + " /data/local/tmp/");
            cmd("chmod 755 /data/local/tmp/libnl_menu.so /data/local/tmp/injector");
            log("[+] Files copied");
            
            log("[*] Launching game...");
            cmd("monkey -p com.axlebolt.standoff2 -c android.intent.category.LAUNCHER 1");
            
            log("[*] Waiting 12s...");
            Thread.sleep(12000);
            
            log("[*] Getting PID...");
            String pid = cmd("pidof com.axlebolt.standoff2");
            log("> PID: " + pid);
            if (pid == null || pid.trim().isEmpty()) { log("[!] Game not running."); return; }
            
            log("[*] Injecting...");
            String res = cmd("/data/local/tmp/injector " + pid.trim() + " /data/local/tmp/libnl_menu.so 2>&1");
            log("> " + res);
            
            log("[+] Finished. Check game screen.");
        } catch (Exception e) {
            log("[!] Error: " + e.getMessage());
        } finally {
            runOnUiThread(() -> startBtn.setEnabled(true));
        }
    }
    
    private String cmd(String c) throws Exception {
        out.writeBytes(c + "\n"); out.writeBytes("echo __DONE__\n"); out.flush();
        StringBuilder sb = new StringBuilder(); String line;
        while ((line = in.readLine()) != null) {
            if (line.equals("__DONE__")) break;
            sb.append(line).append("\n");
        }
        return sb.toString().trim();
    }
    
    private void copyAsset(String n, File f) throws Exception {
        InputStream is = getAssets().open(n);
        FileOutputStream os = new FileOutputStream(f);
        byte[] b = new byte[8192]; int r;
        while ((r = is.read(b)) > 0) os.write(b, 0, r);
        os.close(); is.close();
    }
}

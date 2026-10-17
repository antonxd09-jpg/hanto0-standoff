package com.exodium.launcher;

import android.app.Activity;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.WindowManager;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.Toast;
import java.io.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class MainActivity extends Activity {
    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final Handler uiHandler = new Handler(Looper.getMainLooper());
    private WindowManager windowManager;
    private WebView menuView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        windowManager = (WindowManager) getSystemService(WINDOW_SERVICE);
        
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);

        Button startBtn = new Button(this);
        startBtn.setText("СТАРТ");
        startBtn.setOnClickListener(v -> executor.submit(this::startProcess));

        Button stopBtn = new Button(this);
        stopBtn.setText("СТОП");
        stopBtn.setOnClickListener(v -> {
            stopOverlay();
            toast("Остановлено.");
        });

        layout.addView(startBtn);
        layout.addView(stopBtn);
        setContentView(layout);
    }

    private String runSu(String cmd) {
        try {
            Process p = Runtime.getRuntime().exec(new String[]{"su", "-c", cmd});
            BufferedReader r = new BufferedReader(new InputStreamReader(p.getInputStream()));
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = r.readLine()) != null) sb.append(line).append("\n");
            p.waitFor();
            return sb.toString().trim();
        } catch (Exception e) { return "ERROR: " + e.getMessage(); }
    }

    private void extractAsset(String name, File outFile) {
        try (InputStream is = getAssets().open(name); FileOutputStream fos = new FileOutputStream(outFile)) {
            byte[] buf = new byte[8192]; int len;
            while ((len = is.read(buf)) > 0) fos.write(buf, 0, len);
        } catch (Exception ignored) {}
    }

    private void startProcess() {
        try {
            if (!runSu("id -u").equals("0")) { toast("Нет рут-прав!"); return; }

            String pidOut = runSu("pidof com.axlebolt.standoff2");
            if (pidOut.isEmpty() || pidOut.startsWith("ERROR")) {
                toast("Запускаю Standoff 2...");
                runSu("monkey -p com.axlebolt.standoff2 -c android.intent.category.LAUNCHER 1");
                Thread.sleep(8000);
                pidOut = runSu("pidof com.axlebolt.standoff2");
                if (pidOut.isEmpty()) { toast("Игра не запустилась."); return; }
            }
            
            String gamePid = pidOut.split("\\s+")[0];
            String uidStr = runSu("awk '/^Uid:/{print $2}' /proc/" + gamePid + "/status");
            long uid = Long.parseLong(uidStr.trim());
            String gameFiles = "/data/user/" + (uid / 100000) + "/com.axlebolt.standoff2/files";
            String selinux = runSu("ls -Zd " + gameFiles + " | awk '{print $1}'");

            String runtime = "/data/local/tmp/exodium-local";
            runSu("mkdir -p " + runtime + " && chmod 700 " + runtime);

            String nativeDir = getApplicationInfo().nativeLibraryDir;
            runSu("cp " + nativeDir + "/libinjector.so " + runtime + "/manualmap_injector");
            runSu("cp " + nativeDir + "/libmain.so " + runtime + "/libmain.so");

            File restFile = new File(runtime, "rest.sh");
            File cacheFile = new File(runtime, "cache");
            extractAsset("rest.sh", restFile);
            extractAsset("cache", cacheFile);
            runSu("chmod 700 " + runtime + "/manualmap_injector " + runtime + "/rest.sh");
            runSu("chmod 600 " + runtime + "/libmain.so " + runtime + "/cache");

            String exoDir = gameFiles + "/exodium";
            runSu("mkdir -p " + exoDir);
            runSu("chown " + uid + ":" + uid + " " + exoDir + " && chmod 700 " + exoDir + " && chcon " + selinux + " " + exoDir);
            runSu("cp " + runtime + "/cache " + exoDir + "/cache");
            runSu("chown " + uid + ":" + uid + " " + exoDir + "/cache && chmod 600 " + exoDir + "/cache && chcon " + selinux + " " + exoDir + "/cache");

            String[] sFiles = {"sssss3","sssss4","sssss5","sssss","sssss2","sssss7","sssss8","sssss9","sssss1","sssss10"};
            for (String s : sFiles) {
                String f = gameFiles + "/" + s + ".txt";
                runSu("touch " + f + " && chown " + uid + ":" + uid + " " + f + " && chmod 600 " + f + " && chcon " + selinux + " " + f);
            }

            runSu("printf 'Remaster' > /data/local/tmp/version.txt && chmod 644 /data/local/tmp/version.txt");

            toast("Внедряю чит...");
            runSu(runtime + "/manualmap_injector " + gamePid + " " + runtime + "/libmain.so");

            String restCmd = "cd " + runtime + " && nohup /system/bin/sh -c 'EXODIUM_READY_FILE=\"$1/rest.ready\" \"$1/rest.sh\" > \"$1/rest.log\" 2>&1' exodium-worker " + runtime + " > /dev/null 2>&1 &";
            runSu(restCmd);

            Thread.sleep(2000);
            toast("Чит активен! Открываю меню...");
            
            uiHandler.post(this::showOverlay);

        } catch (Exception e) { toast("Ошибка: " + e.getMessage()); }
    }

    private void showOverlay() {
        if (menuView != null) return;
        
        menuView = new WebView(this);
        menuView.getSettings().setJavaScriptEnabled(true);
        menuView.setWebViewClient(new WebViewClient());
        menuView.loadUrl("http://127.0.0.1:8080"); 

        WindowManager.LayoutParams params = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.WRAP_CONTENT,
                WindowManager.LayoutParams.WRAP_CONTENT,
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
                PixelFormat.TRANSLUCENT
        );
        params.gravity = Gravity.TOP | Gravity.START;
        params.x = 100; params.y = 100;

        try {
            windowManager.addView(menuView, params);
        } catch (Exception e) {
            toast("Ошибка оверлея: " + e.getMessage());
        }
    }

    private void stopOverlay() {
        if (menuView != null && windowManager != null) {
            windowManager.removeView(menuView);
            menuView = null;
        }
    }

    private void toast(String msg) {
        uiHandler.post(() -> Toast.makeText(MainActivity.this, msg, Toast.LENGTH_SHORT).show());
    }
}

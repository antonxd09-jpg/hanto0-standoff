package com.exodium.launcher;

import android.app.Activity;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.view.Gravity;
import android.view.WindowManager;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import java.io.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class MainActivity extends Activity {
    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final Handler uiHandler = new Handler(Looper.getMainLooper());
    private TextView logView;
    private Process currentProcess;
    private WindowManager windowManager;
    private WebView menuView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        windowManager = (WindowManager) getSystemService(WINDOW_SERVICE);
        
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(30, 30, 30, 30);

        Button startBtn = new Button(this);
        startBtn.setText("СТАРТ");
        startBtn.setOnClickListener(v -> executor.submit(this::startProcess));

        Button stopBtn = new Button(this);
        stopBtn.setText("СТОП");
        stopBtn.setOnClickListener(v -> stopProcess());

        logView = new TextView(this);
        logView.setTextColor(0xFF00FF00);
        logView.setBackgroundColor(0xFF111111);
        logView.setPadding(20, 20, 20, 20);
        logView.setTextSize(12);

        ScrollView scroll = new ScrollView(this);
        scroll.addView(logView);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f);
        scroll.setLayoutParams(params);

        layout.addView(startBtn);
        layout.addView(stopBtn);
        layout.addView(scroll);
        setContentView(layout);
    }

    private void appendLog(String msg) {
        uiHandler.post(() -> {
            logView.append(msg + "\n");
            int scrollAmount = logView.getLayout().getLineTop(logView.getLineCount()) - logView.getHeight();
            if (scrollAmount > 0) logView.scrollTo(0, scrollAmount);
            else logView.scrollTo(0, 0);
        });
    }

    private String runSu(String cmd) {
        try {
            ProcessBuilder pb = new ProcessBuilder("su", "-c", cmd);
            pb.redirectErrorStream(true);
            Process p = pb.start();
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
        } catch (Exception e) { 
            appendLog("ОШИБКА АССЕТА " + name + ": " + e.getMessage()); 
        }
    }

    private void startProcess() {
        try {
            if (!runSu("id -u").equals("0")) { appendLog("Нет рут-прав!"); return; }
            
            if (!Settings.canDrawOverlays(this)) {
                appendLog("Запрашиваю разрешение на оверлей...");
                Intent intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                        Uri.parse("package:" + getPackageName()));
                startActivity(intent);
                return;
            }

            appendLog("Подготовка файлов...");
            String runtime = "/data/local/tmp/hantoo-local";
            runSu("mkdir -p " + runtime + " && chmod 700 " + runtime);

            String nativeDir = getApplicationInfo().nativeLibraryDir;
            runSu("cp " + nativeDir + "/libinjector.so " + runtime + "/manualmap_injector");
            runSu("cp " + nativeDir + "/libmain.so " + runtime + "/libmain.so");

            File cacheDir = getCacheDir();
            File startSh = new File(cacheDir, "start.sh");
            File stopSh = new File(cacheDir, "stop.sh");
            File restSh = new File(cacheDir, "rest.sh");
            File cacheBin = new File(cacheDir, "cache");

            extractAsset("start.sh", startSh);
            extractAsset("stop.sh", stopSh);
            extractAsset("rest.sh", restSh);
            extractAsset("cache", cacheBin);

            runSu("cp " + startSh.getAbsolutePath() + " " + runtime + "/start.sh");
            runSu("cp " + stopSh.getAbsolutePath() + " " + runtime + "/stop.sh");
            runSu("cp " + restSh.getAbsolutePath() + " " + runtime + "/rest.sh");
            runSu("cp " + cacheBin.getAbsolutePath() + " " + runtime + "/cache");

            runSu("chmod 700 " + runtime + "/manualmap_injector " + runtime + "/start.sh " + runtime + "/stop.sh " + runtime + "/rest.sh");
            runSu("chmod 600 " + runtime + "/libmain.so " + runtime + "/cache");

            appendLog("Запуск start.sh (ожидание инжекта)...");
            
            String cmd = "cd " + runtime + " && setsid sh start.sh";
            ProcessBuilder pb = new ProcessBuilder("su", "-c", cmd);
            pb.redirectErrorStream(true);
            currentProcess = pb.start();
            
            BufferedReader r = new BufferedReader(new InputStreamReader(currentProcess.getInputStream()));
            String line;
            while ((line = r.readLine()) != null) {
                appendLog(line);
            }
            currentProcess.waitFor();
            appendLog("Скрипт завершил работу. Открываю меню...");

            // Автоматический вызов меню после успешного старта
            uiHandler.post(this::showOverlay);

        } catch (Exception e) {
            appendLog("Ошибка: " + e.getMessage());
        }
    }

    private void showOverlay() {
        if (menuView != null) return;
        
        menuView = new WebView(this);
        menuView.getSettings().setJavaScriptEnabled(true);
        menuView.setWebViewClient(new WebViewClient());
        menuView.setBackgroundColor(0x80000000); // Полупрозрачный фон
        // Порт по умолчанию 8080. Если не работает - замени на нужный.
        menuView.loadUrl("http://127.0.0.1:8080"); 

        WindowManager.LayoutParams params = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
                PixelFormat.TRANSLUCENT
        );
        params.gravity = Gravity.TOP | Gravity.START;
        params.x = 0; params.y = 0;

        try {
            windowManager.addView(menuView, params);
            appendLog("Меню открыто автоматически.");
        } catch (Exception e) {
            appendLog("Ошибка оверлея: " + e.getMessage());
        }
    }

    private void stopProcess() {
        executor.submit(() -> {
            appendLog("Остановка...");
            if (menuView != null && windowManager != null) {
                windowManager.removeView(menuView);
                menuView = null;
            }
            String runtime = "/data/local/tmp/hantoo-local";
            runSu("cd " + runtime + " && sh stop.sh");
            if (currentProcess != null) currentProcess.destroy();
            appendLog("Остановлено.");
        });
    }
}

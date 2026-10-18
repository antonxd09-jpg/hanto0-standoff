package com.exodium.launcher;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.View;
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
    private ScrollView scrollView;
    private Process currentProcess;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setBackgroundColor(Color.parseColor("#0B0C10")); // Темный фон
        layout.setPadding(60, 120, 60, 60);

        TextView title = new TextView(this);
        title.setText("HantoO");
        title.setTextColor(Color.parseColor("#66FCF1")); // Неоновый
        title.setTextSize(40);
        title.setTypeface(null, Typeface.BOLD);
        title.setGravity(Gravity.CENTER);
        title.setPadding(0, 0, 0, 100);
        layout.addView(title);

        LinearLayout.LayoutParams btnParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, 180);
        btnParams.setMargins(0, 0, 0, 50);

        // Кнопка СТАРТ
        Button startBtn = new Button(this);
        startBtn.setText("ЗАПУСТИТЬ");
        startBtn.setTextColor(Color.WHITE);
        startBtn.setTextSize(22);
        startBtn.setTypeface(null, Typeface.BOLD);
        startBtn.setBackground(createGradient("#00B4DB", "#0083B0"));
        startBtn.setOnClickListener(v -> executor.submit(this::startProcess));
        startBtn.setLayoutParams(btnParams);
        layout.addView(startBtn);

        // Кнопка СТОП
        Button stopBtn = new Button(this);
        stopBtn.setText("ОСТАНОВИТЬ");
        stopBtn.setTextColor(Color.WHITE);
        stopBtn.setTextSize(22);
        stopBtn.setTypeface(null, Typeface.BOLD);
        stopBtn.setBackground(createGradient("#FF416C", "#FF4B2B"));
        stopBtn.setOnClickListener(v -> stopProcess());
        stopBtn.setLayoutParams(btnParams);
        layout.addView(stopBtn);

        // Окно логов
        logView = new TextView(this);
        logView.setTextColor(Color.parseColor("#00FFCC")); 
        logView.setTextSize(13);
        logView.setPadding(30, 30, 30, 30);

        GradientDrawable logBg = new GradientDrawable();
        logBg.setColor(Color.parseColor("#1F2833"));
        logBg.setCornerRadius(20);
        logBg.setStroke(3, Color.parseColor("#45A29E"));
        logView.setBackground(logBg);

        scrollView = new ScrollView(this);
        scrollView.addView(logView);
        LinearLayout.LayoutParams scrollParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f);
        scrollView.setLayoutParams(scrollParams);
        
        layout.addView(scrollView);
        setContentView(layout);

        appendLog("HantoO UI Loaded.");
    }

    private GradientDrawable createGradient(String colorTop, String colorBottom) {
        GradientDrawable gd = new GradientDrawable(
                GradientDrawable.Orientation.TL_BR,
                new int[] {Color.parseColor(colorTop), Color.parseColor(colorBottom)});
        gd.setCornerRadius(40);
        return gd;
    }

    private void appendLog(String msg) {
        uiHandler.post(() -> {
            if (logView != null) {
                logView.append("> " + msg + "\n");
                // Безопасная прокрутка вниз
                scrollView.post(() -> scrollView.fullScroll(View.FOCUS_DOWN));
            }
        });
    }

    private String runSu(String cmd) {
        try {
            ProcessBuilder pb = new ProcessBuilder("su", "-mm", "-c", cmd);
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
            appendLog("ОШИБКА " + name + ": " + e.getMessage()); 
        }
    }

    private void startProcess() {
        try {
            if (!runSu("id -u").equals("0")) { appendLog("Нет рут-прав!"); return; }

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

            appendLog("Запуск start.sh...");
            
            String cmd = "cd " + runtime + " && sh start.sh";
            ProcessBuilder pb = new ProcessBuilder("su", "-mm", "-c", cmd);
            pb.redirectErrorStream(true);
            currentProcess = pb.start();
            
            BufferedReader r = new BufferedReader(new InputStreamReader(currentProcess.getInputStream()));
            String line;
            while ((line = r.readLine()) != null) {
                appendLog(line);
            }
            int exitCode = currentProcess.waitFor();
            if (exitCode == 0) {
                appendLog("Успешно. Чит в игре!");
            } else {
                appendLog("Ошибка скрипта (код " + exitCode + ").");
            }

        } catch (Exception e) {
            appendLog("Ошибка: " + e.getMessage());
        }
    }

    private void stopProcess() {
        executor.submit(() -> {
            appendLog("Остановка...");
            String runtime = "/data/local/tmp/hantoo-local";
            runSu("cd " + runtime + " && sh stop.sh");
            if (currentProcess != null) currentProcess.destroy();
            appendLog("Остановлено.");
        });
    }
}

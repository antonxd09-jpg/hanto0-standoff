const canvas = document.getElementById('gameCanvas');
const ctx = canvas.getContext('2d');
const startScreen = document.getElementById('start-screen');
const gameOverScreen = document.getElementById('game-over');
const audioUpload = document.getElementById('audio-upload');
const startBtn = document.getElementById('start-btn');
const restartBtn = document.getElementById('restart-btn');

let width, height;
function resize() { width = canvas.width = document.getElementById('game-container').clientWidth; height = canvas.height = document.getElementById('game-container').clientHeight; }
window.addEventListener('resize', resize); resize();

let audioCtx, audioEl, analyser, dataArray, source;
let isPlaying = false;
let lastBeatTime = 0;
const BEAT_COOLDOWN = 400; // Минимум 400мс между нотами
const BASS_THRESHOLD = 130; // Чувствительность (чем больше, тем реже спавн)

// История басов для детекции именно удара (кика)
let bassHistory = [];

let state = 'menu', score = 0, combo = 0, lives = 3, nodes = [], particles = [], gameLoopId;
let currentAudioUrl = null;
let screenPulse = 0;

class Node {
    constructor(x, y, time) {
        this.x = x; this.y = y; this.radius = 45;
        this.startTime = time; 
        // УВЕЛИЧИЛИ ДЛИТЕЛЬНОСТЬ: теперь кольцо летит 1.2 секунды
        this.duration = 1200; 
        this.hitTime = this.startTime + this.duration;
        this.isHit = false; this.isMissed = false;
        this.color = Math.random() > 0.5 ? '#00f3ff' : '#ff00ff';
    }
    draw(currentTime) {
        if (this.isHit || this.isMissed) return;
        const progress = (currentTime - this.startTime) / this.duration;
        // Кольцо начинается шире и сжимается плавнее
        const ringRadius = this.radius + (1 - progress) * 80;
        
        ctx.beginPath(); ctx.arc(this.x, this.y, Math.max(0, ringRadius), 0, Math.PI * 2); 
        ctx.strokeStyle = this.color; ctx.lineWidth = 5; ctx.stroke();
        
        ctx.beginPath(); ctx.arc(this.x, this.y, this.radius, 0, Math.PI * 2); 
        ctx.fillStyle = 'rgba(255,255,255,0.1)'; ctx.fill();
        
        ctx.beginPath(); ctx.arc(this.x, this.y, 8, 0, Math.PI * 2); 
        ctx.fillStyle = this.color; ctx.fill();
    }
}

class Particle {
    constructor(x, y, color) { 
        this.x = x; this.y = y; const a = Math.random() * Math.PI * 2; const s = Math.random() * 6 + 3; 
        this.vx = Math.cos(a) * s; this.vy = Math.sin(a) * s; this.life = 1; this.color = color; this.size = Math.random() * 4 + 2; 
    }
    update() { this.x += this.vx; this.y += this.vy; this.vx *= 0.9; this.vy *= 0.9; this.life -= 0.04; }
    draw() { ctx.globalAlpha = this.life; ctx.fillStyle = this.color; ctx.beginPath(); ctx.arc(this.x, this.y, this.size, 0, Math.PI * 2); ctx.fill(); ctx.globalAlpha = 1; }
}

// Звук тапа
function playTapSound(isPerfect) {
    if (!audioCtx) return;
    const osc = audioCtx.createOscillator();
    const gain = audioCtx.createGain();
    osc.connect(gain); gain.connect(audioCtx.destination);
    const now = audioCtx.currentTime;
    osc.type = 'sine';
    osc.frequency.setValueAtTime(isPerfect ? 1200 : 800, now);
    gain.gain.setValueAtTime(0.1, now);
    gain.gain.exponentialRampToValueAtTime(0.001, now + 0.1);
    osc.start(now); osc.stop(now + 0.1);
}

startBtn.addEventListener('click', () => audioUpload.click());
restartBtn.addEventListener('click', () => { gameOverScreen.classList.add('hidden'); audioUpload.click(); });

audioUpload.addEventListener('change', (e) => {
    const file = e.target.files[0];
    if (!file) return;
    if (currentAudioUrl) URL.revokeObjectURL(currentAudioUrl);
    currentAudioUrl = URL.createObjectURL(file);
    startGame(currentAudioUrl);
});

function startGame(url) {
    if (audioCtx) { audioEl.pause(); audioCtx.close(); }

    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    audioEl = new Audio(url);
    source = audioCtx.createMediaElementSource(audioEl);
    analyser = audioCtx.createAnalyser();
    analyser.fftSize = 256;
    source.connect(analyser);
    analyser.connect(audioCtx.destination);
    
    dataArray = new Uint8Array(analyser.frequencyBinCount);
    bassHistory = [];

    audioEl.play();
    audioEl.onended = () => endGame();

    startScreen.classList.add('hidden');
    gameOverScreen.classList.add('hidden');
    
    state = 'playing'; isPlaying = true;
    score = 0; combo = 0; lives = 3; nodes = []; particles = [];
    updateUI();
    
    if (gameLoopId) cancelAnimationFrame(gameLoopId);
    gameLoop();
}

function updateUI() {
    document.getElementById('score-display').innerText = `DATA: ${score}`;
    document.getElementById('combo-display').innerText = `COMBO: x${combo}`;
    document.getElementById('lives-display').innerText = `SYS: [${'💚'.repeat(lives)}${'🖤'.repeat(3 - lives)}]`;
}

function triggerGlitch() {
    const container = document.getElementById('game-container');
    container.classList.add('shake');
    setTimeout(() => container.classList.remove('shake'), 300);
    canvas.style.backgroundColor = 'rgba(255,0,0,0.2)';
    setTimeout(() => canvas.style.backgroundColor = 'transparent', 100);
}

function spawnNote(currentTime) {
    const padding = 100;
    let x, y;
    do {
        x = Math.random() * (width - padding * 2) + padding;
        y = Math.random() * (height - padding * 2) + padding;
    } while (nodes.some(n => Math.hypot(n.x - x, n.y - y) < 100));
    nodes.push(new Node(x, y, currentTime));
}

function checkTap(x, y) {
    if (state !== 'playing') return;
    const currentTime = performance.now();
    
    for (let i = nodes.length - 1; i >= 0; i--) {
        const n = nodes[i];
        if (n.isHit || n.isMissed) continue;

        // Большая зона клика
        if (Math.hypot(x - n.x, y - n.y) < n.radius + 60) {
            const timeDiff = Math.abs(currentTime - n.hitTime);
            n.isHit = true;

            // Окна попадания растянуты под медленное кольцо
            if (timeDiff < 200) { // PERFECT
                combo++; score += 10 * Math.max(1, Math.floor(combo / 5) + 1);
                for (let j = 0; j < 20; j++) particles.push(new Particle(n.x, n.y, n.color));
                playTapSound(true);
            } else if (timeDiff < 350) { // GOOD
                combo++; score += 5;
                for (let j = 0; j < 10; j++) particles.push(new Particle(n.x, n.y, n.color));
                playTapSound(false);
            } else {
                n.isMissed = true;
                combo = 0; lives--;
                triggerGlitch();
            }
            updateUI(); 
            break;
        }
    }
}

function gameLoop() {
    if (state !== 'playing') return;
    const currentTime = performance.now();

    // Анализ музыки (ТОЛЬКО БАСЫ 0-4 бина, игнорируем мелодию)
    analyser.getByteFrequencyData(dataArray);
    let bassSum = 0;
    for (let i = 0; i < 4; i++) bassSum += dataArray[i];
    let currentBass = bassSum / 4;

    // Считаем среднее значение баса за последние кадры
    bassHistory.push(currentBass);
    if (bassHistory.length > 20) bassHistory.shift();
    let avgBass = bassHistory.reduce((a, b) => a + b, 0) / bassHistory.length;

    // Спавним нод, если бас РЕЗКО скакнул вверх (удар барабана)
    let isKick = (currentBass > BASS_THRESHOLD && currentBass > avgBass * 1.2);

    screenPulse *= 0.9;
    if (isKick && currentTime - lastBeatTime > BEAT_COOLDOWN) {
        spawnNote(currentTime);
        lastBeatTime = currentTime;
        screenPulse = currentBass / 255 * 50;
    }

    // Проверка пропущенных
    for (let i = nodes.length - 1; i >= 0; i--) {
        const n = nodes[i];
        if (!n.isHit && !n.isMissed && currentTime > n.hitTime + 400) {
            n.isMissed = true;
            combo = 0;
            lives--;
            triggerGlitch();
            updateUI();
        }
        if (n.isHit || n.isMissed) nodes.splice(i, 1);
    }

    if (lives <= 0) { endGame(); return; }

    // Отрисовка
    ctx.fillStyle = '#050510'; 
    ctx.fillRect(0, 0, width, height);
    
    ctx.strokeStyle = `rgba(0, 243, 255, ${0.05 + screenPulse/500})`; 
    ctx.lineWidth = 1;
    for (let i = 0; i < width; i += 40) { ctx.beginPath(); ctx.moveTo(i, 0); ctx.lineTo(i, height); ctx.stroke(); }
    for (let i = 0; i < height; i += 40) { ctx.beginPath(); ctx.moveTo(0, i); ctx.lineTo(width, i); ctx.stroke(); }

    // Визуализатор (только басы)
    const barWidth = width / 16; 
    for (let i = 0; i < 16; i++) {
        const barHeight = (dataArray[i] / 255) * 120;
        ctx.fillStyle = `rgba(255, 0, 255, ${0.5 + barHeight/240})`; 
        ctx.fillRect(i * barWidth, height - barHeight, barWidth - 2, barHeight);
    }

    for (let i = particles.length - 1; i >= 0; i--) {
        particles[i].update();
        if (particles[i].life <= 0) particles.splice(i, 1);
        else particles[i].draw();
    }

    nodes.forEach(n => n.draw(currentTime));
    gameLoopId = requestAnimationFrame(gameLoop);
}

function endGame() {
    state = 'gameover'; isPlaying = false;
    if (audioEl) audioEl.pause();
    cancelAnimationFrame(gameLoopId);
    
    document.getElementById('final-score').innerText = score;
    gameOverScreen.classList.remove('hidden');
    ctx.fillStyle = '#050510'; ctx.fillRect(0, 0, width, height);
}

canvas.addEventListener('pointerdown', (e) => {
    const rect = canvas.getBoundingClientRect();
    checkTap(e.clientX - rect.left, e.clientY - rect.top);
});

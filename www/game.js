const canvas = document.getElementById('gameCanvas');
const ctx = canvas.getContext('2d');
const startScreen = document.getElementById('start-screen');
const gameOverScreen = document.getElementById('game-over');
let width, height;
function resize() { width = canvas.width = document.getElementById('game-container').clientWidth; height = canvas.height = document.getElementById('game-container').clientHeight; }
window.addEventListener('resize', resize);
resize();
const AudioContext = window.AudioContext || window.webkitAudioContext;
let audioCtx, isBGMPlaying = false;
function initAudio() { if (!audioCtx) audioCtx = new AudioContext(); if (audioCtx.state === 'suspended') audioCtx.resume(); }
function playHitSound(type) {
    if (!audioCtx) return;
    const osc = audioCtx.createOscillator(); const gain = audioCtx.createGain();
    osc.connect(gain); gain.connect(audioCtx.destination); const now = audioCtx.currentTime;
    if (type === 'perfect') { osc.type = 'sine'; osc.frequency.setValueAtTime(880, now); osc.frequency.exponentialRampToValueAtTime(1760, now + 0.1); gain.gain.setValueAtTime(0.3, now); gain.gain.exponentialRampToValueAtTime(0.01, now + 0.2); osc.start(now); osc.stop(now + 0.2); }
    else if (type === 'good') { osc.type = 'triangle'; osc.frequency.setValueAtTime(440, now); gain.gain.setValueAtTime(0.2, now); gain.gain.exponentialRampToValueAtTime(0.01, now + 0.1); osc.start(now); osc.stop(now + 0.1); }
    else if (type === 'miss') { osc.type = 'sawtooth'; osc.frequency.setValueAtTime(150, now); osc.frequency.linearRampToValueAtTime(50, now + 0.3); gain.gain.setValueAtTime(0.4, now); gain.gain.exponentialRampToValueAtTime(0.01, now + 0.3); osc.start(now); osc.stop(now + 0.3); }
}
setInterval(() => { if (isBGMPlaying && audioCtx) { const osc = audioCtx.createOscillator(); const gain = audioCtx.createGain(); osc.connect(gain); gain.connect(audioCtx.destination); const now = audioCtx.currentTime; osc.type = 'square'; osc.frequency.setValueAtTime(60, now); gain.gain.setValueAtTime(0.1, now); gain.gain.exponentialRampToValueAtTime(0.001, now + 0.1); osc.start(now); osc.stop(now + 0.1); } }, 500);
let state = 'menu', score = 0, combo = 0, lives = 3, nodes = [], particles = [], spawnInterval, gameLoopId, spawnDelay = 1200;
class Node {
    constructor() { this.radius = 40; this.x = Math.random() * (width - this.radius * 2) + this.radius; this.y = Math.random() * (height - this.radius * 4) + this.radius + 80; this.startTime = performance.now(); this.duration = 1500; this.hitTime = this.startTime + this.duration; this.isHit = false; this.isMissed = false; this.color = Math.random() > 0.5 ? '#00f3ff' : '#ff00ff'; }
    draw(currentTime) {
        if (this.isHit || this.isMissed) return;
        const progress = (currentTime - this.startTime) / this.duration;
        const ringRadius = this.radius + (1 - progress) * 60;
        ctx.beginPath(); ctx.arc(this.x, this.y, Math.max(0, ringRadius), 0, Math.PI * 2); ctx.strokeStyle = this.color; ctx.lineWidth = 3; ctx.stroke();
        ctx.beginPath(); ctx.arc(this.x, this.y, this.radius, 0, Math.PI * 2); ctx.fillStyle = 'rgba(255,255,255,0.1)'; ctx.fill();
        ctx.beginPath(); ctx.arc(this.x, this.y, 5, 0, Math.PI * 2); ctx.fillStyle = this.color; ctx.fill();
    }
}
class Particle {
    constructor(x, y, color) { this.x = x; this.y = y; const angle = Math.random() * Math.PI * 2; const speed = Math.random() * 5 + 2; this.vx = Math.cos(angle) * speed; this.vy = Math.sin(angle) * speed; this.life = 1; this.color = color; this.size = Math.random() * 3 + 1; }
    update() { this.x += this.vx; this.y += this.vy; this.vx *= 0.95; this.vy *= 0.95; this.life -= 0.03; }
    draw() { ctx.globalAlpha = this.life; ctx.fillStyle = this.color; ctx.beginPath(); ctx.arc(this.x, this.y, this.size, 0, Math.PI * 2); ctx.fill(); ctx.globalAlpha = 1; }
}
function startGame() {
    initAudio(); startScreen.classList.add('hidden'); gameOverScreen.classList.add('hidden');
    state = 'playing'; isBGMPlaying = true; score = 0; combo = 0; lives = 3; nodes = []; particles = []; spawnDelay = 1200;
    updateUI(); spawnNode();
    if (gameLoopId) cancelAnimationFrame(gameLoopId); gameLoop();
}
function spawnNode() { if (state !== 'playing') return; nodes.push(new Node()); spawnDelay = Math.max(400, spawnDelay - 20); setTimeout(spawnNode, spawnDelay); }
function updateUI() { document.getElementById('score-display').innerText = `DATA: ${score}`; document.getElementById('combo-display').innerText = `COMBO: x${combo}`; document.getElementById('lives-display').innerText = `SYS: [${'💚'.repeat(lives)}${'🖤'.repeat(3 - lives)}]`; }
function triggerGlitch() { const container = document.getElementById('game-container'); container.classList.add('shake'); setTimeout(() => container.classList.remove('shake'), 300); canvas.style.backgroundColor = 'rgba(255,0,0,0.1)'; setTimeout(() => canvas.style.backgroundColor = 'transparent', 100); }
function checkTap(x, y) {
    if (state !== 'playing') return;
    const currentTime = performance.now(); let tappedSomething = false;
    for (let i = nodes.length - 1; i >= 0; i--) {
        const n = nodes[i]; if (n.isHit || n.isMissed) continue;
        if (Math.hypot(x - n.x, y - n.y) < n.radius + 30) {
            tappedSomething = true; const timeDiff = Math.abs(currentTime - n.hitTime);
            if (timeDiff < 150) { n.isHit = true; combo++; score += 10 * Math.max(1, Math.floor(combo / 5) + 1); playHitSound('perfect'); for(let j=0; j<20; j++) particles.push(new Particle(n.x, n.y, n.color)); }
            else if (timeDiff < 300) { n.isHit = true; combo++; score += 5; playHitSound('good'); for(let j=0; j<10; j++) particles.push(new Particle(n.x, n.y, n.color)); }
            else { n.isMissed = true; combo = 0; lives--; playHitSound('miss'); triggerGlitch(); }
            updateUI(); break;
        }
    }
    if (!tappedSomething) { combo = 0; updateUI(); }
}
function gameLoop() {
    if (state !== 'playing') return;
    const currentTime = performance.now();
    for (let i = nodes.length - 1; i >= 0; i--) {
        const n = nodes[i];
        if (!n.isHit && !n.isMissed && currentTime > n.hitTime + 300) { n.isMissed = true; combo = 0; lives--; playHitSound('miss'); triggerGlitch(); updateUI(); }
        if (n.isHit || n.isMissed) nodes.splice(i, 1);
    }
    if (lives <= 0) { endGame(); return; }
    for (let i = particles.length - 1; i >= 0; i--) { particles[i].update(); if (particles[i].life <= 0) particles.splice(i, 1); }
    ctx.fillStyle = '#050510'; ctx.fillRect(0, 0, width, height);
    ctx.strokeStyle = 'rgba(0, 243, 255, 0.05)'; ctx.lineWidth = 1;
    for (let i = 0; i < width; i += 40) { ctx.beginPath(); ctx.moveTo(i, 0); ctx.lineTo(i, height); ctx.stroke(); }
    for (let i = 0; i < height; i += 40) { ctx.beginPath(); ctx.moveTo(0, i); ctx.lineTo(width, i); ctx.stroke(); }
    nodes.forEach(n => n.draw(currentTime)); particles.forEach(p => p.draw());
    gameLoopId = requestAnimationFrame(gameLoop);
}
function endGame() {
    state = 'gameover'; isBGMPlaying = false; cancelAnimationFrame(gameLoopId);
    document.getElementById('final-score').innerText = score;
    gameOverScreen.classList.remove('hidden');
    ctx.fillStyle = '#050510'; ctx.fillRect(0, 0, width, height);
}
canvas.addEventListener('pointerdown', (e) => { const rect = canvas.getBoundingClientRect(); checkTap(e.clientX - rect.left, e.clientY - rect.top); });
document.getElementById('start-btn').addEventListener('click', startGame);
document.getElementById('restart-btn').addEventListener('click', startGame);

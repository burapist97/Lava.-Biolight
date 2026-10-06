#pragma once
// BIOLIGHT kontrol paneli — ESP32 tarafından http://biolight.local adresinde sunulur.
// Arayüzün renkleri günün evresine (Şafak / Gün / Alacakaranlık / Gece) göre değişir.

const char INDEX_HTML[] PROGMEM = R"BL(<!DOCTYPE html>
<html lang="tr"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#120805">
<meta name="apple-mobile-web-app-capable" content="yes">
<title>Biolight</title>
<link rel="icon" href="/favicon.svg">
<link rel="apple-touch-icon" href="/favicon.svg">
<style>
:root{--bg:#120805;--bg2:#2a1208;--tile:rgba(255,190,140,.04);--line:rgba(255,190,140,.10);--text:#d99a62;--muted:#7f5638;--accent:#d9631e;--ink:#120805;--glow:rgba(217,99,30,.35);color-scheme:dark}
body[data-phase=safak]{--bg:#1c1222;--bg2:#5a2a38;--tile:rgba(255,220,200,.05);--line:rgba(255,220,200,.12);--text:#ffe6d4;--muted:#c99d8d;--accent:#ff9d66;--ink:#1c1222;--glow:rgba(255,157,102,.45)}
body[data-phase=gun]{--bg:#f7f1e6;--bg2:#ffdca0;--tile:rgba(255,255,255,.6);--line:rgba(70,45,10,.12);--text:#2b1f13;--muted:#8a735a;--accent:#ff8c00;--ink:#fff;--glow:rgba(255,140,0,.35);color-scheme:light}
body[data-phase=alacakaranlik]{--bg:#1e0d09;--bg2:#66220f;--tile:rgba(255,210,180,.05);--line:rgba(255,210,180,.11);--text:#ffd9bd;--muted:#bb816a;--accent:#ff6a2a;--ink:#1e0d09;--glow:rgba(255,106,42,.45)}
*{box-sizing:border-box;margin:0;padding:0}
[hidden]{display:none!important}
html{background:var(--bg)}
body{min-height:100vh;color:var(--text);background:radial-gradient(130% 55% at 50% 0%,var(--bg2),var(--bg) 72%) no-repeat var(--bg);font:15px/1.5 -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;transition:color 1.2s;padding:env(safe-area-inset-top) 0 env(safe-area-inset-bottom);-webkit-tap-highlight-color:transparent}
.app{max-width:460px;margin:0 auto;padding:18px 18px 48px}
button,input,select{font:inherit;color:inherit}
button{cursor:pointer}
:focus-visible{outline:2px solid var(--accent);outline-offset:2px}
header{display:flex;align-items:center;gap:12px}
.brand b{display:block;font-size:16px;letter-spacing:.34em;font-weight:700}
.brand small{display:block;color:var(--muted);font-size:12.5px}
#pwr{margin-left:auto;width:48px;height:48px;border-radius:50%;border:1px solid var(--line);background:transparent;color:var(--muted);display:grid;place-items:center;transition:background .3s,box-shadow .3s,color .3s}
#pwr.on{background:var(--accent);color:var(--ink);border-color:transparent;box-shadow:0 0 28px var(--glow)}
.dial{position:relative;width:100%;max-width:330px;margin:14px auto 4px;aspect-ratio:1}
.dial svg{position:absolute;inset:0;width:100%;height:100%;overflow:visible}
.center{position:absolute;inset:0;display:flex;flex-direction:column;align-items:center;justify-content:center;text-align:center;pointer-events:none}
.time{font-size:54px;font-weight:200;line-height:1;font-variant-numeric:tabular-nums;letter-spacing:.01em}
.phase{margin-top:8px;font-size:15px;color:var(--accent);font-weight:600}
.wx{margin-top:6px;font-size:13px;color:var(--muted);max-width:170px;line-height:1.35}
.trk{fill:none;stroke:var(--line);stroke-width:10}
.day{fill:none;stroke:url(#dg);stroke-width:10;stroke-linecap:round;opacity:.95;transition:opacity 1s}
body[data-phase=gece] .day{opacity:.3}
.tk{stroke:var(--muted);stroke-width:1;opacity:.55}.tk.mj{stroke-width:1.8;opacity:1}
.hl{fill:var(--muted);font-size:10px;text-anchor:middle;dominant-baseline:middle}
.sk{stroke:var(--text);stroke-width:2;stroke-linecap:round;opacity:.8}
.sun{display:flex;justify-content:space-between;max-width:330px;margin:0 auto;font-size:12.5px;color:var(--muted)}
.sd{fill:var(--bg);stroke:var(--text);stroke-width:1.6}
.nh{fill:var(--accent);opacity:.7}.nd{fill:var(--accent);stroke:var(--bg);stroke-width:2.5}
section{margin-top:26px;padding-top:20px;border-top:1px solid var(--line)}
section:first-of-type{border-top:none;padding-top:0}
h2{font-size:17px;font-weight:600;margin-bottom:4px}
.hint{color:var(--muted);font-size:13px;margin-bottom:12px}
.modes{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-top:12px}
.mode{border:1px solid var(--line);background:var(--tile);border-radius:16px;padding:13px 6px 11px;font-size:13px;display:flex;flex-direction:column;align-items:center;gap:8px;transition:border-color .25s,box-shadow .25s}
.mode i{width:26px;height:26px;border-radius:50%;display:block}
.mode.wide{grid-column:span 3;flex-direction:row;padding:14px 16px;gap:14px;text-align:left;border-radius:20px}
.mode.wide i{width:38px;height:38px;flex:none}
.mode.wide b{display:block;font-size:15px;font-weight:600}
.mode.wide small{display:block;color:var(--muted);font-size:12.5px}
.mode.on{border-color:var(--accent);box-shadow:inset 0 0 0 1px var(--accent),0 0 22px var(--glow)}
.row{display:flex;align-items:center;gap:14px;margin:12px 0}
.row>label{width:76px;flex:none;font-size:14px;color:var(--muted)}
input[type=range]{flex:1;accent-color:var(--accent);height:30px;min-width:0}
.cols{display:flex;gap:12px}
input[type=color]{-webkit-appearance:none;appearance:none;width:42px;height:42px;border:2px solid var(--line);border-radius:50%;padding:0;background:none;overflow:hidden}
input[type=color]::-webkit-color-swatch-wrapper{padding:0}
input[type=color]::-webkit-color-swatch{border:none;border-radius:50%}
input[type=color]::-moz-color-swatch{border:none;border-radius:50%}
.sw{-webkit-appearance:none;appearance:none;width:44px;height:26px;border-radius:13px;background:var(--line);position:relative;flex:none;transition:background .25s;border:none}
.sw::after{content:"";position:absolute;top:3px;left:3px;width:20px;height:20px;border-radius:50%;background:var(--muted);transition:left .25s,background .25s}
.sw:checked{background:var(--accent)}.sw:checked::after{left:21px;background:var(--ink)}
.toggle{display:flex;align-items:center;gap:14px;justify-content:space-between}
.chips{display:flex;flex-wrap:wrap;gap:7px;margin-top:10px}
.chip{border:1px solid var(--line);background:transparent;border-radius:999px;padding:7px 13px;font-size:13.5px;transition:background .2s}
.chip.on{background:var(--accent);color:var(--ink);border-color:transparent}
.now{font-size:13.5px;margin-top:10px}
.now b{font-weight:600}
.rule{display:flex;gap:8px;align-items:center;margin-bottom:9px}
.field{background:var(--tile);border:1px solid var(--line);border-radius:12px;padding:9px 11px;font-size:15px;min-width:0}
.rule select{flex:1}
select option{background:var(--bg);color:var(--text)}
.x{background:none;border:none;color:var(--muted);font-size:24px;width:34px;height:34px;flex:none;line-height:1}
.btns{display:flex;gap:8px;margin-top:6px;flex-wrap:wrap}
.btn{background:var(--accent);color:var(--ink);border:none;border-radius:12px;padding:11px 16px;font-weight:600;font-size:14.5px}
.btn.ghost{background:transparent;color:var(--text);border:1px solid var(--line);font-weight:500}
.btn:disabled{opacity:.4}
.city{display:flex;gap:8px}
.city .field{flex:1}
.status{font-size:13px;color:var(--muted);margin-top:8px}
.status.err{color:#ff7a59}
footer{margin-top:34px;text-align:center;color:var(--muted);font-size:12px;line-height:1.7}
.rooms{display:flex;gap:8px;overflow-x:auto;margin:16px -18px 0;padding:2px 18px;scrollbar-width:none}
.rooms::-webkit-scrollbar{display:none}
.room{flex:none;display:flex;align-items:center;gap:7px;border:1px solid var(--line);background:var(--tile);border-radius:999px;padding:8px 15px;font-size:14px;color:var(--text);text-decoration:none;white-space:nowrap}
.room.on{background:var(--accent);color:var(--ink);border-color:transparent;font-weight:600}
.room svg{opacity:.75}
.peers{margin-top:14px}
.peer{display:flex;align-items:center;gap:12px;padding:11px 0;border-top:1px solid var(--line);font-size:14.5px}
.peer span{flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.peer small{color:var(--muted);font-size:13px}
.peer a{color:var(--accent);font-size:14px;text-decoration:none;font-weight:600}
.tl{font-size:15px;font-weight:600}
.welcome{position:relative;margin-top:18px;padding:15px 44px 15px 16px;border-radius:18px;background:var(--tile);border:1px solid var(--accent);box-shadow:0 0 26px -8px var(--glow)}
.welcome b{display:block;font-size:15.5px;margin-bottom:3px}
.welcome p{color:var(--muted);font-size:13.5px}
.welcome .x{position:absolute;top:8px;right:6px}
.toast{position:fixed;left:50%;bottom:calc(22px + env(safe-area-inset-bottom));transform:translate(-50%,20px);background:var(--text);color:var(--bg);padding:10px 16px;border-radius:12px;font-size:14px;opacity:0;transition:opacity .25s,transform .25s;pointer-events:none;max-width:90vw}
.toast.show{opacity:1;transform:translate(-50%,0)}
@media (prefers-reduced-motion:reduce){*{transition:none!important}}
</style>
</head>
<body data-phase="gece">
<div class="app">
<header>
  <svg width="40" height="40" viewBox="0 0 100 100" aria-hidden="true"><circle cx="50" cy="50" r="38" fill="none" stroke="var(--accent)" stroke-width="5"/><circle cx="50" cy="50" r="27" fill="none" stroke="var(--accent)" stroke-width="2" stroke-dasharray="4 6" opacity=".6"/><circle cx="50" cy="50" r="14" fill="var(--accent)"/></svg>
  <div class="brand"><b>BIOLIGHT</b><small id="devName">Sirkadiyen aydınlatma</small></div>
  <button id="pwr" aria-label="Işığı aç veya kapat"><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"><path d="M12 3v8"/><path d="M6.3 6.8a8 8 0 1 0 11.4 0"/></svg></button>
</header>

<nav class="rooms" id="rooms" hidden aria-label="Biolight ışıkların"></nav>

<div class="welcome" id="welcome" hidden>
  <b>Hoş geldin, Biolight hazır.</b>
  <p id="wTip"></p>
  <button class="x" id="wClose" aria-label="Kapat">×</button>
</div>

<div class="dial">
  <svg id="dial" viewBox="0 0 300 300" aria-hidden="true"></svg>
  <div class="center">
    <div class="time" id="time">--:--</div>
    <div class="phase" id="phase">Bağlanıyor</div>
    <div class="wx" id="wxline"></div>
  </div>
</div>
<div class="sun"><span id="sr"></span><span id="ss"></span></div>

<section>
  <h2>Işık modu</h2>
  <div class="modes" id="modes"></div>
</section>

<section>
  <h2>Ayarlar</h2>
  <div class="row"><label for="bri">Parlaklık</label><input type="range" id="bri" min="8" max="255"></div>
  <div class="row" id="colRow"><label>Renkler</label><div class="cols"><input type="color" id="c0" aria-label="Renk 1"><input type="color" id="c1" aria-label="Renk 2"><input type="color" id="c2" aria-label="Renk 3"></div></div>
  <div class="row" id="spdRow"><label for="spd">Hız</label><input type="range" id="spd" min="1" max="10"></div>
</section>

<section>
  <div class="toggle"><div><h2>Gökyüzü efektleri</h2><p class="hint" style="margin:0">Sirkadiyen modda ışık, penceredeki gökyüzünü yansıtır.</p></div><input type="checkbox" class="sw" id="wfx" aria-label="Gökyüzü efektleri"></div>
  <p class="now" id="wxnow"></p>
  <p class="hint" style="margin:14px 0 0">Bir efekte dokunarak 45 saniye boyunca önizleyebilirsin.</p>
  <div class="chips" id="chips"></div>
</section>

<section>
  <h2>Zamanlama</h2>
  <p class="hint">Belirlediğin saatte ışık seçtiğin moda geçer. Kurallar her gün tekrarlanır.</p>
  <div id="rules"></div>
  <div class="btns"><button class="btn ghost" id="addRule">Kural ekle</button><button class="btn" id="saveSched" hidden>Zamanlamayı kaydet</button></div>
</section>

<section>
  <h2>Konum</h2>
  <p class="hint">Gün doğumu, gün batımı ve hava durumu bu şehre göre hesaplanır.</p>
  <div class="city"><input class="field" id="city" placeholder="Örn. Ankara veya Berlin,DE" autocomplete="off"><button class="btn" id="saveCity">Kaydet</button></div>
  <p class="status" id="cityStatus"></p>
</section>

<section>
  <h2>Bu ışık</h2>
  <p class="hint">Birden fazla Biolight kullanıyorsan her birine bulunduğu odanın adını ver.</p>
  <div class="city"><input class="field" id="name" maxlength="31" placeholder="Örn. Salon" autocomplete="off"><button class="btn" id="saveName">Kaydet</button></div>
  <div class="toggle" style="margin-top:22px">
    <div><p class="tl">Birlikte çalış</p><p class="hint" style="margin:2px 0 0">Bu seçenek açık olan tüm Biolight'lar aynı modda, renkte, parlaklıkta ve zamanlamada çalışır.</p></div>
    <input type="checkbox" class="sw" id="sync" aria-label="Birlikte çalış">
  </div>
  <div class="peers" id="peers"></div>
</section>

<section>
  <h2>Wi-Fi</h2>
  <p class="hint">Cihazı başka bir ağa bağlamak için Wi-Fi ayarlarını sıfırla. Biolight yeniden başlar ve “LAVA_Setup” ağını açar. Panele ulaşamadığında aynı işlemi cihazdaki BOOT düğmesine 5 saniye basılı tutarak da yapabilirsin.</p>
  <button class="btn ghost" id="wifiReset">Wi-Fi ayarlarını sıfırla</button>
</section>

<footer>Biolight, bir LAVA. ürünüdür.<br>Hava verisi OpenWeather tarafından sağlanır.<br><span id="ip"></span></footer>
</div>
<div class="toast" id="toast"></div>

<script>
const $=s=>document.querySelector(s);
const MODES=[["sirkadiyen","Sirkadiyen","Gün ışığının ritmini ve havayı izler"],["mum","Mum"],["sabit","Sabit renk"],["gokkusagi","Gökkuşağı"],["akis","Renk akışı"],["gecis","Renk geçişi"],["nefes","Nefes"]];
const SMODES=MODES.map(m=>[m[0],m[1]]).concat([["kapali","Kapalı"]]);
const WX=[["gunes","Güneşli"],["parcali","Parçalı bulutlu"],["bulutlu","Bulutlu"],["sis","Sisli"],["yagmur","Yağmur"],["saganak","Sağanak ve şimşek"],["kar","Kar"]];
const WXN=Object.fromEntries(WX);
const COLS={sabit:1,nefes:1,akis:3,gecis:3};
const SPD={gokkusagi:1,akis:1,gecis:1,nefes:1};
const THEME={gece:"#120805",safak:"#1c1222",gun:"#f7f1e6",alacakaranlik:"#1e0d09"};
let st=null,busy=0,sched=[],schedDirty=false;

const pad=n=>String(n).padStart(2,"0");
const hm=m=>{m=((Math.round(m)%1440)+1440)%1440;return pad(Math.floor(m/60))+":"+pad(m%60)};
const deb=(fn,ms)=>{let t;return(...a)=>{clearTimeout(t);t=setTimeout(()=>fn(...a),ms)}};
function toast(t){const e=$("#toast");e.textContent=t;e.classList.add("show");clearTimeout(e._t);e._t=setTimeout(()=>e.classList.remove("show"),2400)}
async function api(p,body){const o=body===undefined?{}:{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(body)};const r=await fetch(p,o);if(!r.ok)throw new Error(r.status);return r.json()}
function send(o){busy=Date.now()+1500;api("/api/set",o).then(apply).catch(()=>toast("Biolight'a ulaşılamadı. Aynı Wi-Fi ağında olduğundan emin ol."))}
const sendDeb=deb(send,140);

function icon(k,c){
  if(k=="sirkadiyen")return"radial-gradient(circle at 50% 62%,#fff4d8,#ffb46e 42%,#ff6a2a 70%,#5a1a08)";
  if(k=="mum")return"radial-gradient(circle at 50% 68%,#ffe08a,#ff8a1f 48%,#7a2606)";
  if(k=="sabit")return c[0];
  if(k=="gokkusagi")return"conic-gradient(#ff3b30,#ffcc00,#34c759,#32ade6,#5856d6,#ff2d92,#ff3b30)";
  if(k=="akis")return`conic-gradient(${c[0]},${c[1]},${c[2]},${c[0]})`;
  if(k=="gecis")return`linear-gradient(135deg,${c[0]},${c[1]},${c[2]})`;
  return`radial-gradient(circle,${c[0]} 32%,transparent 74%)`;
}
function buildModes(){
  const box=$("#modes");
  MODES.forEach(([k,n,d])=>{
    const b=document.createElement("button");b.className="mode"+(d?" wide":"");b.dataset.k=k;
    b.innerHTML=d?`<i></i><span><b>${n}</b><small>${d}</small></span>`:`<i></i>${n}`;
    b.onclick=()=>{document.querySelectorAll(".mode").forEach(x=>x.classList.toggle("on",x==b));send({mode:k})};
    box.appendChild(b);
  });
  WX.forEach(([k,n])=>{
    const c=document.createElement("button");c.className="chip";c.dataset.k=k;c.textContent=n;
    c.onclick=()=>send({preview:c.classList.contains("on")?"":k});
    $("#chips").appendChild(c);
  });
}

function P(min,r){const a=min/1440*2*Math.PI;return[150-r*Math.sin(a),150+r*Math.cos(a)]}
const f1=n=>n.toFixed(1);
function drawDial(s){
  const R=112,sr=s.sr,ss=s.ss,[x1,y1]=P(sr,R),[x2,y2]=P(ss,R),span=((ss-sr)+1440)%1440;
  let h=`<defs><linearGradient id="dg" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#fff0c4"/><stop offset=".5" stop-color="#ffb347"/><stop offset="1" stop-color="#ff6a2a"/></linearGradient><filter id="gl" x="-2" y="-2" width="5" height="5"><feGaussianBlur stdDeviation="5"/></filter></defs>`;
  h+=`<circle class="trk" cx="150" cy="150" r="${R}"/>`;
  h+=`<path class="day" d="M${f1(x1)} ${f1(y1)} A${R} ${R} 0 ${span>720?1:0} 1 ${f1(x2)} ${f1(y2)}"/>`;
  for(let i=0;i<24;i++){
    const m=i*60,mj=i%6==0,[a,b]=P(m,R-14),[c,d]=P(m,R-(mj?24:18));
    h+=`<line class="tk${mj?" mj":""}" x1="${f1(a)}" y1="${f1(b)}" x2="${f1(c)}" y2="${f1(d)}"/>`;
    if(mj){const[tx,ty]=P(m,R-34);h+=`<text class="hl" x="${f1(tx)}" y="${f1(ty)}">${pad(i)}</text>`}
  }
  [sr,ss].forEach(m=>{const[a,b]=P(m,R-6),[c,d]=P(m,R+9);h+=`<line class="sk" x1="${f1(a)}" y1="${f1(b)}" x2="${f1(c)}" y2="${f1(d)}"/>`});
  $("#sr").textContent="Gün doğumu "+hm(sr);$("#ss").textContent="Gün batımı "+hm(ss);
  sched.forEach(r=>{if(!r.on)return;const[x,y]=P(r.h*60+r.m,R);h+=`<circle class="sd" cx="${f1(x)}" cy="${f1(y)}" r="4"/>`});
  if(s.tv){const[x,y]=P(s.now+s.sec/60,R);h+=`<circle class="nh" cx="${f1(x)}" cy="${f1(y)}" r="9" filter="url(#gl)"/><circle class="nd" cx="${f1(x)}" cy="${f1(y)}" r="7.5"/>`}
  $("#dial").innerHTML=h;
}

function apply(s){
  st=s;
  document.body.dataset.phase=s.phase;
  document.querySelector('meta[name=theme-color]').content=THEME[s.phase]||"#120805";
  $("#pwr").classList.toggle("on",s.pwr);
  $("#time").textContent=s.tv?hm(s.now):"--:--";
  $("#phase").textContent=s.phaseName;
  const w=s.wx;
  $("#wxline").textContent=w.ok?`${w.name}, ${Math.round(w.temp)}°\n${w.desc}`:(w.err||"Hava durumu alınıyor");
  $("#wxline").style.whiteSpace="pre-line";
  document.querySelectorAll(".mode").forEach(b=>{b.classList.toggle("on",s.pwr&&b.dataset.k==s.mode);b.querySelector("i").style.background=icon(b.dataset.k,s.c)});
  if(Date.now()>busy){
    $("#bri").value=s.bri;$("#spd").value=s.spd;$("#wfx").checked=s.wfx;
    s.c.forEach((c,i)=>{const e=$("#c"+i);if(document.activeElement!==e)e.value=c.toLowerCase()});
  }
  const n=COLS[s.mode]||0;
  $("#colRow").hidden=!n;["#c0","#c1","#c2"].forEach((id,i)=>$(id).hidden=i>=n);
  $("#spdRow").hidden=!SPD[s.mode];
  let t;
  if(s.preview)t=`Önizleniyor: <b>${WXN[s.preview]}</b>`;
  else if(!w.ok)t=w.err?`Hava durumu alınamadı: ${w.err}`:"Hava durumu alınıyor.";
  else if(!s.wfx)t="Gökyüzü efektleri kapalı. Işık yalnızca günün saatine göre değişir.";
  else{
    const nm=WXN[w.cat]||"standart",quiet=s.phase=="gece"&&!["yagmur","saganak","kar"].includes(w.cat);
    t=quiet?`Gökyüzü ${nm.toLocaleLowerCase("tr")}. Gece olduğu için mum ışığı yanıyor.`:`Şu an <b>${nm}</b> efekti gösteriliyor.`;
    if(s.mode!="sirkadiyen")t+=" Görmek için Sirkadiyen moda geç.";
  }
  $("#wxnow").innerHTML=t;
  document.querySelectorAll(".chip").forEach(c=>c.classList.toggle("on",c.dataset.k==s.preview));
  if(!schedDirty){sched=s.sched.map(r=>({...r}));renderSched()}
  if(document.activeElement!==$("#city"))$("#city").value=s.city;
  const cs=$("#cityStatus");
  cs.textContent=w.err?w.err:(w.ok?`Bulunan konum: ${w.name}`:"");cs.classList.toggle("err",!!w.err);
  $("#ip").textContent=s.ip?`Bu ışığın adresi: http://${s.host||"biolight"}.local veya http://${s.ip}`:"";
  $("#devName").textContent=s.name;
  document.title=s.name+" · Biolight";
  if(document.activeElement!==$("#name"))$("#name").value=s.name;
  if(Date.now()>busy)$("#sync").checked=s.sync;
  renderRooms(s);
  drawDial(s);
}

const LINK='<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round" aria-label="Birlikte çalışıyor"><path d="M10 14a4 4 0 0 0 5.7 0l3-3a4 4 0 0 0-5.7-5.7l-1 1"/><path d="M14 10a4 4 0 0 0-5.7 0l-3 3a4 4 0 0 0 5.7 5.7l1-1"/></svg>';
function renderRooms(s){
  const nav=$("#rooms"),all=[{name:s.name,me:1,sync:s.sync}].concat(s.peers);
  nav.hidden=!s.peers.length;
  nav.innerHTML=all.map(p=>p.me?`<span class="room on">${esc(p.name)}${p.sync?LINK:""}</span>`:`<a class="room" href="http://${p.ip}/">${esc(p.name)}${p.sync?LINK:""}</a>`).join("");
  const box=$("#peers");
  box.innerHTML=s.peers.length?s.peers.map(p=>`<div class="peer"><span>${esc(p.name)}</span><small>${p.sync?"Birlikte çalışıyor":"Bağımsız"}</small><a href="http://${p.ip}/">Aç</a></div>`).join(""):'<p class="hint" style="margin:0">Ağında başka Biolight bulunmadı. Yeni bir ışık kurduğunda burada görünür.</p>';
}
const esc=t=>String(t).replace(/[&<>"']/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"}[c]));

function renderSched(){
  const box=$("#rules");box.innerHTML="";
  if(!sched.length)box.innerHTML='<p class="hint">Henüz kural yok. Örneğin 07:00 Sirkadiyen, 22:30 Mum, 00:30 Kapalı.</p>';
  sched.forEach((r,i)=>{
    const d=document.createElement("div");d.className="rule";
    d.innerHTML=`<input type="checkbox" class="sw" aria-label="Kuralı etkinleştir" ${r.on?"checked":""}><input type="time" class="field" value="${hm(r.h*60+r.m)}"><select class="field">${SMODES.map(m=>`<option value="${m[0]}"${m[0]==r.mode?" selected":""}>${m[1]}</option>`).join("")}</select><button class="x" aria-label="Kuralı sil">×</button>`;
    const[cb,tm,sel,x]=d.children;
    cb.onchange=()=>{r.on=cb.checked;markSched()};
    tm.onchange=()=>{const[h,m]=tm.value.split(":").map(Number);if(!isNaN(h)&&!isNaN(m)){r.h=h;r.m=m;markSched()}};
    sel.onchange=()=>{r.mode=sel.value;markSched()};
    x.onclick=()=>{sched.splice(i,1);markSched();renderSched()};
    box.appendChild(d);
  });
  $("#addRule").disabled=sched.length>=10;
}
function markSched(){schedDirty=true;$("#saveSched").hidden=false;if(st)drawDial(st)}

$("#pwr").onclick=()=>send({pwr:!(st&&st.pwr)});
$("#bri").oninput=e=>sendDeb({bri:+e.target.value});
$("#spd").oninput=e=>sendDeb({spd:+e.target.value});
$("#wfx").onchange=e=>send({wfx:e.target.checked});
["#c0","#c1","#c2"].forEach(id=>$(id).oninput=()=>{busy=Date.now()+1500;sendDeb({c:[$("#c0").value,$("#c1").value,$("#c2").value]})});
$("#saveName").onclick=()=>{const n=$("#name").value.trim();if(!n){$("#name").focus();return}$("#name").blur();send({name:n});toast("Ad kaydedildi")};
$("#name").onkeydown=e=>{if(e.key=="Enter")$("#saveName").click()};
$("#sync").onchange=e=>{
  const on=e.target.checked;send({sync:on});
  if(on&&st&&st.peers.some(p=>p.sync)){toast("Grubun ayarları alınıyor");setTimeout(poll,2500)}
  else toast(on?"Birlikte çalışma açıldı":"Bu ışık artık bağımsız çalışıyor");
};
$("#addRule").onclick=()=>{sched.push({on:true,h:22,m:30,mode:"mum"});markSched();renderSched()};
$("#saveSched").onclick=()=>{
  sched.sort((a,b)=>(a.h*60+a.m)-(b.h*60+b.m));
  api("/api/sched",sched).then(s=>{schedDirty=false;$("#saveSched").hidden=true;apply(s);toast("Zamanlama kaydedildi")}).catch(()=>toast("Zamanlama kaydedilemedi. Bağlantını kontrol et."));
};
$("#saveCity").onclick=()=>{
  const c=$("#city").value.trim();if(!c){toast("Bir şehir adı yaz");return}
  $("#city").blur();
  api("/api/city",{city:c}).then(s=>{apply(s);toast("Konum kaydedildi, hava durumu güncelleniyor");setTimeout(poll,3500)}).catch(()=>toast("Konum kaydedilemedi"));
};
$("#wifiReset").onclick=()=>{
  if(!confirm("Wi-Fi ayarları silinecek ve Biolight kurulum moduna geçecek. Devam edilsin mi?"))return;
  api("/api/wifireset",{}).then(()=>toast("Biolight yeniden başlıyor. “LAVA_Setup” ağına bağlan.")).catch(()=>{});
};

if(new URLSearchParams(location.search).has("hosgeldin")){
  const ios=/iPhone|iPad|iPod/.test(navigator.userAgent);
  $("#wTip").textContent=ios?"Bu paneli uygulama gibi açmak için Safari’de Paylaş düğmesine, ardından “Ana Ekrana Ekle”ye dokun.":"Bu paneli uygulama gibi açmak için tarayıcının menüsünden “Ana ekrana ekle”ye dokun.";
  $("#welcome").hidden=false;
  history.replaceState(null,"","/");
}
$("#wClose").onclick=()=>$("#welcome").hidden=true;

function poll(){api("/api/state").then(apply).catch(()=>{})}
buildModes();poll();setInterval(poll,4000);
</script>
</body></html>
)BL";

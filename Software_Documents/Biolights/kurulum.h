#pragma once
// BIOLIGHT ilk kurulum sayfası — "LAVA_Setup" ağına bağlanan müşteriyi karşılar.
// Sayfa adım adım ilerledikçe arka plandaki güneş doğar: gece → şafak → gün.

const char KURULUM_HTML[] PROGMEM = R"BL(<!DOCTYPE html>
<html lang="tr"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#120805">
<title>Biolight kurulumu</title>
<style>
:root{--bg:#120805;--text:#ecb88c;--muted:#8d6249;--accent:#ff8c00;--ink:#120805;--tile:rgba(255,200,160,.05);--line:rgba(255,200,160,.13);--sun:#c2410c;--sunY:96%;--sunO:.45;color-scheme:dark}
body[data-theme=safak]{--bg:#1c1222;--text:#ffe6d4;--muted:#c99d8d;--accent:#ff9d66;--ink:#1c1222;--tile:rgba(255,225,205,.06);--line:rgba(255,225,205,.14);--sun:#ff8a4c;--sunY:66%;--sunO:.55}
body[data-theme=gun]{--bg:#f7f1e6;--text:#2b1f13;--muted:#86705a;--accent:#ff8c00;--ink:#fff;--tile:rgba(255,255,255,.7);--line:rgba(70,45,10,.13);--sun:#ffd27a;--sunY:-12%;--sunO:.75;color-scheme:light}
*{box-sizing:border-box;margin:0;padding:0}
[hidden]{display:none!important}
html,body{min-height:100%}
body{background-color:var(--bg);color:var(--text);font:16px/1.5 -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;transition:background-color 1.8s,color 1.8s;-webkit-tap-highlight-color:transparent;overflow-x:hidden}
.sun{position:fixed;left:50%;top:var(--sunY);width:560px;height:560px;margin-left:-280px;border-radius:50%;background-color:var(--sun);opacity:var(--sunO);filter:blur(100px);transition:top 2.4s cubic-bezier(.25,.8,.25,1),background-color 2.4s,opacity 2.4s;z-index:0;pointer-events:none}
.app{position:relative;z-index:1;max-width:420px;margin:0 auto;min-height:100vh;display:flex;flex-direction:column;padding:calc(22px + env(safe-area-inset-top)) 22px calc(22px + env(safe-area-inset-bottom))}
button,input{font:inherit;color:inherit}
button{cursor:pointer}
:focus-visible{outline:2px solid var(--accent);outline-offset:2px}
.brand{display:flex;align-items:center;gap:11px}
.brand b{display:block;letter-spacing:.34em;font-size:15px}
.brand small{display:block;color:var(--muted);font-size:12.5px}
.prog{display:flex;gap:6px;margin-left:auto}
.prog i{width:22px;height:4px;border-radius:2px;background:var(--line);transition:background .6s}
.prog i.on{background:var(--accent)}
main{flex:1;display:flex;flex-direction:column;justify-content:center;padding:36px 0 24px}
.screen{animation:in .5s ease both}
@keyframes in{from{opacity:0;transform:translateY(10px)}to{opacity:1;transform:none}}
h1{font-size:30px;line-height:1.15;font-weight:300;letter-spacing:-.01em;margin-bottom:12px}
.lede{color:var(--muted);font-size:16px;margin-bottom:26px;max-width:34ch}
.btn{display:block;width:100%;text-align:center;text-decoration:none;background:var(--accent);color:var(--ink);border:none;border-radius:16px;padding:16px;font-weight:600;font-size:16px;box-shadow:0 10px 30px -10px var(--accent);transition:transform .15s,opacity .2s}
.btn:active{transform:scale(.98)}
.btn:disabled{opacity:.45;box-shadow:none}
.btn.ghost{background:transparent;color:var(--text);border:1px solid var(--line);box-shadow:none;font-weight:500}
.link{background:none;border:none;color:var(--muted);font-size:14.5px;text-decoration:underline;text-underline-offset:3px;padding:10px 0}
.note{color:var(--muted);font-size:13.5px;margin-top:14px}
.err{background:rgba(255,95,60,.12);border:1px solid rgba(255,95,60,.35);color:inherit;border-radius:14px;padding:12px 14px;font-size:14.5px;margin-bottom:16px}
.logo{width:120px;height:120px;margin-bottom:26px}
.logo .r1{animation:spin 26s linear infinite;transform-origin:50% 50%}
.logo .core{animation:pulse 3.5s ease-in-out infinite;transform-origin:50% 50%}
@keyframes spin{to{transform:rotate(360deg)}}
@keyframes pulse{50%{transform:scale(1.12);opacity:.85}}
.list{display:flex;flex-direction:column;gap:8px;margin-bottom:12px}
.net{border:1px solid var(--line);background:var(--tile);border-radius:16px;overflow:hidden;transition:border-color .2s}
.net.sel{border-color:var(--accent)}
.net>button{width:100%;display:flex;align-items:center;gap:12px;padding:14px 16px;background:none;border:none;text-align:left}
.net .nm{flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;font-size:16px}
.net svg{flex:none;color:var(--muted)}
.pw{padding:0 16px 16px}
.field{width:100%;background:var(--tile);border:1px solid var(--line);border-radius:12px;padding:13px 14px;font-size:16px}
.pwrow{position:relative;margin-bottom:12px}
.pwrow .field{padding-right:72px}
.eye{position:absolute;right:6px;top:50%;transform:translateY(-50%);background:none;border:none;color:var(--muted);font-size:13.5px;padding:8px 10px}
.empty{color:var(--muted);font-size:14.5px;padding:18px 4px}
.row2{display:flex;justify-content:space-between;align-items:center;margin-top:6px}
.chips{display:flex;flex-wrap:wrap;gap:7px;margin:12px 0 22px}
.chip{border:1px solid var(--line);background:transparent;border-radius:999px;padding:8px 14px;font-size:14.5px}
.chip.on{background:var(--accent);color:var(--ink);border-color:transparent}
.checks{list-style:none;margin:6px 0 22px}
.checks li{display:flex;gap:14px;align-items:center;padding:12px 0;color:var(--muted);transition:color .4s}
.checks li.act,.checks li.ok{color:var(--text)}
.dot{width:26px;height:26px;border-radius:50%;border:2px solid var(--line);flex:none;display:grid;place-items:center;transition:all .4s}
.act .dot{border-color:var(--accent);border-top-color:transparent;animation:spin 1s linear infinite}
.ok .dot{background:var(--accent);border-color:var(--accent);color:var(--ink)}
.ok .dot::after{content:"";width:9px;height:5px;border:2.5px solid currentColor;border-top:none;border-right:none;transform:rotate(-45deg) translate(1px,-1px)}
.card{border:1px solid var(--line);background:var(--tile);border-radius:20px;padding:18px;margin-bottom:12px}
.wxrow{display:grid;grid-template-columns:1fr 1fr 1fr;gap:6px;text-align:center}
.wxrow b{display:block;font-size:20px;font-weight:400;font-variant-numeric:tabular-nums}
.wxrow small{color:var(--muted);font-size:12.5px}
.addr small{color:var(--muted);font-size:13.5px}
.addr b{display:block;font-size:26px;font-weight:500;letter-spacing:.01em;margin:2px 0}
.addr .ip{color:var(--muted);font-size:14px;font-variant-numeric:tabular-nums}
.addr button{margin-top:10px;font-size:14px;padding:8px 12px;border-radius:10px;border:1px solid var(--line);background:none}
ol.steps{margin:18px 0 22px 0;padding-left:22px;color:var(--muted);font-size:15px}
ol.steps li{margin:7px 0;padding-left:4px}
ol.steps li::marker{color:var(--accent);font-weight:600}
.toggle{display:flex;align-items:center;gap:14px;justify-content:space-between}
.sw{-webkit-appearance:none;appearance:none;width:46px;height:28px;border-radius:14px;background:var(--line);position:relative;flex:none;transition:background .25s;border:none}
.sw::after{content:"";position:absolute;top:3px;left:3px;width:22px;height:22px;border-radius:50%;background:var(--muted);transition:left .25s,background .25s}
.sw:checked{background:var(--accent)}.sw:checked::after{left:21px;background:var(--ink)}
.big{width:72px;height:72px;margin-bottom:20px}
.wait{width:56px;height:56px;margin-bottom:22px;border-radius:50%;border:3px solid var(--line);border-top-color:var(--accent);animation:spin 1.1s linear infinite}
.stack{display:flex;flex-direction:column;gap:10px}
footer{text-align:center;color:var(--muted);font-size:12px}
@media (prefers-reduced-motion:reduce){*{animation:none!important;transition:none!important}}
</style>
</head>
<body data-theme="gece">
<div class="sun"></div>
<div class="app">
<header class="brand">
  <svg width="36" height="36" viewBox="0 0 100 100" aria-hidden="true"><circle cx="50" cy="50" r="38" fill="none" stroke="var(--accent)" stroke-width="5"/><circle cx="50" cy="50" r="27" fill="none" stroke="var(--accent)" stroke-width="2" stroke-dasharray="4 6" opacity=".6"/><circle cx="50" cy="50" r="14" fill="var(--accent)"/></svg>
  <div><b>BIOLIGHT</b><small>Sirkadiyen aydınlatma</small></div>
  <div class="prog" id="prog" hidden aria-hidden="true"><i></i><i></i><i></i><i></i></div>
</header>

<main>
<!-- 0 · Karşılama -->
<section class="screen" id="s0">
  <svg class="logo" viewBox="0 0 100 100" aria-hidden="true"><circle cx="50" cy="50" r="40" fill="none" stroke="var(--accent)" stroke-width="3"/><circle class="r1" cx="50" cy="50" r="29" fill="none" stroke="var(--accent)" stroke-width="1.6" stroke-dasharray="4 6" opacity=".7"/><circle class="core" cx="50" cy="50" r="15" fill="var(--accent)"/></svg>
  <h1>Işığın, gün doğumunu bekliyor.</h1>
  <p class="lede">Biolight gün ışığının ritmini evine taşır. Bunun için önce Wi-Fi ağına bağlanması gerekiyor. Kurulum bir dakikadan kısa sürer.</p>
  <button class="btn" id="start">Kuruluma başla</button>
  <p class="note">Wi-Fi şifren elinin altında olsun.</p>
</section>

<!-- 1 · Ağ seçimi -->
<section class="screen" id="s1" hidden>
  <h1>Evinin Wi-Fi ağını seç</h1>
  <p class="lede">Biolight 2.4 GHz ağlara bağlanır. Listede yalnızca bağlanabileceği ağlar görünür.</p>
  <div class="err" id="err1" hidden></div>
  <div class="list" id="nets"><p class="empty">Ağlar aranıyor…</p></div>
  <div class="net" id="manual" hidden>
    <div class="pw" style="padding-top:16px">
      <input class="field" id="mSsid" placeholder="Ağ adı" autocomplete="off" autocapitalize="off" style="margin-bottom:10px">
      <div class="pwrow"><input class="field" id="mPass" type="password" placeholder="Şifre" autocomplete="off"><button class="eye" type="button">Göster</button></div>
      <button class="btn" id="mGo">Devam et</button>
    </div>
  </div>
  <div class="row2"><button class="link" id="rescan">Listeyi yenile</button><button class="link" id="showManual">Ağım listede yok</button></div>
</section>

<!-- 2 · Şehir -->
<section class="screen" id="s2" hidden>
  <h1>Hangi şehirdesin?</h1>
  <p class="lede">Biolight gün doğumunu, gün batımını ve havayı bu şehre göre izler.</p>
  <input class="field" id="city" placeholder="Şehir adı" autocomplete="off">
  <div class="chips" id="cities"></div>
  <button class="btn" id="toName">Devam et</button>
  <p class="note">Yurt dışındaysan ülke kodunu ekle, örneğin Berlin,DE.</p>
  <button class="link" id="back2">Ağ seçimine dön</button>
</section>

<!-- 6 · Işığın adı -->
<section class="screen" id="s6" hidden>
  <h1>Bu ışık nerede duracak?</h1>
  <p class="lede">Birden fazla Biolight kullanırsan onları odalarına göre ayırt edersin.</p>
  <input class="field" id="dname" placeholder="Örn. Salon" maxlength="31" autocomplete="off">
  <div class="chips" id="rooms"></div>
  <button class="btn" id="connect">Bağlan</button>
  <button class="link" id="back6">Şehir seçimine dön</button>
</section>

<!-- 3 · Bağlanıyor -->
<section class="screen" id="s3" hidden>
  <h1>Biolight evine bağlanıyor</h1>
  <ul class="checks">
    <li id="c1"><span class="dot"></span><span id="c1t">Wi-Fi ağına bağlanıyor</span></li>
    <li id="c2"><span class="dot"></span><span id="c2t">İnternet ve konum doğrulanıyor</span></li>
    <li id="c3"><span class="dot"></span><span>Işığın hazırlanıyor</span></li>
  </ul>
  <p class="note" id="n3">Bu sırada telefonun birkaç saniyeliğine kurulum ağından düşebilir. Sayfayı kapatma, kendiliğinden toparlanacak.</p>
</section>

<!-- 4 · Hazır -->
<section class="screen" id="s4" hidden>
  <svg class="big" viewBox="0 0 100 100" aria-hidden="true"><circle cx="50" cy="50" r="22" fill="var(--accent)"/><g stroke="var(--accent)" stroke-width="5" stroke-linecap="round"><path d="M50 8v12M50 80v12M8 50h12M80 50h12M20 20l8 8M72 72l8 8M20 80l8-8M72 28l8-8"/></g></svg>
  <h1>Biolight evine bağlandı.</h1>
  <p class="lede">Halkadaki ışık artık günün saatini ve gökyüzünü takip ediyor.</p>
  <div class="card" id="wxcard">
    <div class="wxrow">
      <div><b id="wTemp">--°</b><small id="wCity">Hava</small></div>
      <div><b id="wSr">--:--</b><small>Gün doğumu</small></div>
      <div><b id="wSs">--:--</b><small>Gün batımı</small></div>
    </div>
  </div>
  <div class="card" id="cityFix" hidden>
    <p style="margin-bottom:10px" id="cityFixT">Şehrini bulamadık. Yazımını kontrol edip tekrar dene.</p>
    <input class="field" id="city2" style="margin-bottom:10px"><button class="btn ghost" id="cityRetry">Şehri güncelle</button>
  </div>
  <div class="card" id="peerCard" hidden>
    <p style="margin-bottom:14px" id="peerT"></p>
    <div class="toggle"><span>Bu ışık onlarla birlikte çalışsın</span><input type="checkbox" class="sw" id="psync" aria-label="Birlikte çalış"></div>
  </div>
  <div class="card addr">
    <small>Işığını buradan yöneteceksin</small>
    <b id="hostT">biolight.local</b>
    <div class="ip" id="ipT"></div>
    <div class="ip" id="hostNote" hidden style="margin-top:6px">Evindeki tüm Biolight'lara biolight.local adresinden de ulaşabilir, panelin üstündeki oda sekmeleriyle aralarında geçiş yapabilirsin.</div>
    <button id="copy">Adresi kopyala</button>
  </div>
  <ol class="steps">
    <li>Aşağıdaki düğmeye dokun, kurulum ağı kapanır.</li>
    <li>Telefonun ev Wi-Fi ağına kendiliğinden döner.</li>
    <li>Kontrol paneli açılır. Açılmazsa tarayıcına biolight.local yaz.</li>
  </ol>
  <button class="btn" id="finish">Kurulumu tamamla</button>
</section>

<!-- 5 · Ev ağına dönüş -->
<section class="screen" id="s5" hidden>
  <div class="wait" id="spin"></div>
  <h1>Ev ağına dönülüyor</h1>
  <p class="lede">Kurulum ağı kapandı. Telefonun ev Wi-Fi ağına bağlanınca kontrol paneli kendiliğinden açılacak.</p>
  <div id="fallback" hidden>
    <ol class="steps" style="margin-top:0">
      <li>Telefonunun Wi-Fi ayarlarından ev ağını seç.</li>
      <li>Tarayıcını aç ve biolight.local yaz ya da aşağıdaki düğmelerden birine dokun.</li>
    </ol>
    <div class="stack">
      <a class="btn" id="goLocal" href="http://biolight.local/?hosgeldin=1">Kontrol panelini aç</a>
      <a class="btn ghost" id="goIp" href="#">IP adresiyle aç</a>
    </div>
    <p class="note">Bu sayfa telefonun kurulum penceresinde açıldıysa kendiliğinden kapanabilir. Bu normal.</p>
  </div>
</section>
</main>

<footer>Biolight, bir LAVA. ürünüdür.</footer>
</div>

<script>
const $=s=>document.querySelector(s);
const ROOMS=["Salon","Yatak odası","Çalışma odası","Çocuk odası","Mutfak","Antre"];
const THEME={0:"gece",1:"safak",2:"safak",6:"safak",3:"safak",4:"gun",5:"gun"};
const PROG={1:1,2:2,6:3,3:3,4:4};
let readyT=null;
const CITIES=["İstanbul","Ankara","İzmir","Bursa","Antalya","Adana","Konya","Gaziantep"];
let sel=null,ssid="",pass="",ip="",city="",pollT=null,lostSince=0;
const pad=n=>String(n).padStart(2,"0");
const hm=m=>{m=((Math.round(m)%1440)+1440)%1440;return pad(Math.floor(m/60))+":"+pad(m%60)};
const esc=s=>s.replace(/[&<>"']/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"}[c]));

function show(n){
  document.querySelectorAll(".screen").forEach(s=>s.hidden=s.id!="s"+n);
  const th=THEME[n];document.body.dataset.theme=th;
  document.querySelector("meta[name=theme-color]").content={gece:"#120805",safak:"#1c1222",gun:"#f7f1e6"}[th];
  const p=$("#prog");p.hidden=!PROG[n];
  p.querySelectorAll("i").forEach((e,i)=>e.classList.toggle("on",i<(PROG[n]||0)));
  window.scrollTo(0,0);
}
async function api(p,b){const r=await fetch(p,b===undefined?{cache:"no-store"}:{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(b)});if(!r.ok)throw new Error(r.status);return r.json()}

function bars(r){const n=r>-55?4:r>-65?3:r>-75?2:1;let s='<svg width="20" height="16" viewBox="0 0 20 16" aria-hidden="true">';for(let i=0;i<4;i++){const h=4+i*4;s+=`<rect x="${i*5}" y="${16-h}" width="3.4" height="${h}" rx="1" fill="currentColor" opacity="${i<n?1:.25}"/>`}return s+"</svg>"}
const lock='<svg width="14" height="16" viewBox="0 0 14 16" aria-label="Şifreli"><rect x="1" y="7" width="12" height="8.5" rx="2" fill="currentColor"/><path d="M3.5 7V5a3.5 3.5 0 0 1 7 0v2" fill="none" stroke="currentColor" stroke-width="1.8"/></svg>';

function eyes(root){root.querySelectorAll(".eye").forEach(b=>b.onclick=()=>{const i=b.previousElementSibling;const v=i.type=="password";i.type=v?"text":"password";b.textContent=v?"Gizle":"Göster"})}

function renderNets(list){
  const box=$("#nets");box.innerHTML="";
  if(!list.length){box.innerHTML='<p class="empty">Yakında bir ağ bulunamadı. Listeyi yenilemeyi ya da Biolight\'ı modeme yaklaştırmayı dene.</p>';return}
  list.forEach(n=>{
    const d=document.createElement("div");d.className="net";
    d.innerHTML=`<button type="button"><span class="nm">${esc(n.s)}</span>${n.l?lock:""}${bars(n.r)}</button>`+
      `<div class="pw" hidden>${n.l?'<div class="pwrow"><input class="field" type="password" placeholder="Wi-Fi şifresi" autocomplete="off"><button class="eye" type="button">Göster</button></div>':""}<button class="btn go">Devam et</button></div>`;
    const pw=d.querySelector(".pw"),inp=d.querySelector("input");
    d.firstChild.onclick=()=>{
      document.querySelectorAll(".net").forEach(x=>{x.classList.remove("sel");const p=x.querySelector(".pw");if(p&&x!==$("#manual"))p.hidden=true});
      $("#manual").hidden=true;d.classList.add("sel");pw.hidden=false;if(inp)setTimeout(()=>inp.focus(),50);
    };
    const go=()=>{const p=inp?inp.value:"";if(n.l&&p.length<8){inp.focus();inp.style.borderColor="#ff5f3c";return}pick(n.s,p)};
    d.querySelector(".go").onclick=go;
    if(inp)inp.onkeydown=e=>{if(e.key=="Enter")go()};
    eyes(d);box.appendChild(d);
    if(sel&&sel==n.s)d.firstChild.click();
  });
}
async function loadNets(refresh){
  try{
    let r=await api("/api/scan"+(refresh?"?yenile=1":""));
    if(refresh)$("#nets").innerHTML='<p class="empty">Ağlar aranıyor…</p>';
    for(let i=0;i<20&&r.scanning;i++){await new Promise(z=>setTimeout(z,600));r=await api("/api/scan")}
    renderNets(r.list);
  }catch(e){$("#nets").innerHTML='<p class="empty">Ağ listesi alınamadı. Listeyi yenilemeyi dene.</p>'}
}
function pick(s,p){ssid=s;pass=p;sel=s;$("#err1").hidden=true;show(2);if(!$("#city").value)$("#city").focus()}

function setCheck(id,state,text){const e=$(id);e.className=state;if(text)e.querySelector("span:last-child").textContent=text}

async function connect(){
  city=$("#city").value.trim()||"Istanbul";
  show(3);
  setCheck("#c1","act",`${ssid} ağına bağlanıyor`);setCheck("#c2","");setCheck("#c3","");
  const name=$("#dname").value.trim()||"Biolight";
  try{await api("/api/connect",{ssid,pass,city,name})}catch(e){}
  lostSince=0;clearInterval(pollT);pollT=setInterval(poll,1000);
}
async function poll(){
  let s;
  try{s=await api("/api/setup-status");lostSince=0;$("#n3").textContent="Bu sırada telefonun birkaç saniyeliğine kurulum ağından düşebilir. Sayfayı kapatma, kendiliğinden toparlanacak."}
  catch(e){if(!lostSince)lostSince=Date.now();if(Date.now()-lostSince>4000)$("#n3").textContent="Telefonun kurulum ağına yeniden bağlanıyor. Bağlantı kurulamazsa Wi-Fi ayarlarından “LAVA_Setup” ağını tekrar seç.";return}
  if(s.step=="connecting")setCheck("#c1","act");
  if(s.step=="checking"){setCheck("#c1","ok",`${ssid} ağına bağlandı`);setCheck("#c2","act")}
  if(s.step=="done"){
    clearInterval(pollT);
    setCheck("#c1","ok",`${ssid} ağına bağlandı`);
    setCheck("#c2","ok",s.wx.ok&&!s.wx.err?`Konum bulundu: ${s.wx.name}`:"İnternet bağlantısı doğrulandı");
    setCheck("#c3","ok");
    setTimeout(()=>ready(s),900);
  }
  if(s.step=="fail"){
    clearInterval(pollT);show(1);
    const e=$("#err1");e.textContent=s.err;e.hidden=false;loadNets(false);
  }
}
function fillWx(s){
  ip=s.ip||ip;
  $("#ipT").textContent=ip?"veya "+ip:"";
  $("#goIp").href="http://"+ip+"/?hosgeldin=1";
  if(s.wx.ok){$("#wTemp").textContent=Math.round(s.wx.temp)+"°";$("#wCity").textContent=s.wx.name;$("#wSr").textContent=hm(s.sr);$("#wSs").textContent=hm(s.ss)}
  const host=s.host||"biolight";
  $("#hostT").textContent=host+".local";
  $("#hostNote").hidden=host=="biolight";
  const pc=s.peers||[];
  $("#peerCard").hidden=!pc.length;
  if(pc.length){
    $("#peerT").textContent=`Evinde ${pc.length} Biolight daha bulduk: ${pc.map(p=>p.name).join(", ")}.`;
    if(document.activeElement!==$("#psync"))$("#psync").checked=s.sync;
  }
  const bad=!!s.wx.err;$("#cityFix").hidden=!bad;$("#wxcard").hidden=!s.wx.ok;
  if(bad){$("#cityFixT").textContent=s.wx.err;$("#city2").value=city}
}
function ready(s){
  fillWx(s);show(4);
  clearInterval(readyT);readyT=setInterval(()=>api("/api/setup-status").then(fillWx).catch(()=>{}),3000);
}

$("#start").onclick=()=>{show(1);loadNets(false)};
$("#rescan").onclick=()=>loadNets(true);
$("#showManual").onclick=()=>{document.querySelectorAll(".net").forEach(x=>{x.classList.remove("sel");const p=x.querySelector(".pw");if(p&&x.id!="manual")p.hidden=true});$("#manual").hidden=false;$("#mSsid").focus()};
$("#mGo").onclick=()=>{const s=$("#mSsid").value.trim();if(!s){$("#mSsid").focus();return}pick(s,$("#mPass").value)};
eyes($("#manual"));
CITIES.forEach(c=>{const b=document.createElement("button");b.className="chip";b.textContent=c;b.onclick=()=>{$("#city").value=c;document.querySelectorAll("#cities .chip").forEach(x=>x.classList.toggle("on",x==b))};$("#cities").appendChild(b)});
$("#city").oninput=()=>document.querySelectorAll("#cities .chip").forEach(x=>x.classList.toggle("on",x.textContent==$("#city").value));
$("#city").onkeydown=e=>{if(e.key=="Enter")$("#toName").click()};
$("#toName").onclick=()=>{show(6);if(!$("#dname").value)$("#dname").focus()};
$("#back6").onclick=()=>show(2);
$("#connect").onclick=connect;
$("#dname").onkeydown=e=>{if(e.key=="Enter")connect()};
ROOMS.forEach(r=>{const b=document.createElement("button");b.className="chip";b.textContent=r;b.onclick=()=>{$("#dname").value=r;document.querySelectorAll("#rooms .chip").forEach(x=>x.classList.toggle("on",x==b))};$("#rooms").appendChild(b)});
$("#dname").oninput=()=>document.querySelectorAll("#rooms .chip").forEach(x=>x.classList.toggle("on",x.textContent==$("#dname").value));
$("#psync").onchange=e=>api("/api/set",{sync:e.target.checked}).catch(()=>{});
$("#back2").onclick=()=>{show(1);loadNets(false)};
$("#copy").onclick=()=>{const t="http://biolight.local";(navigator.clipboard?navigator.clipboard.writeText(t):Promise.reject()).then(()=>$("#copy").textContent="Kopyalandı").catch(()=>$("#copy").textContent="biolight.local")};
$("#cityRetry").onclick=async()=>{
  const c=$("#city2").value.trim();if(!c)return;city=c;$("#cityRetry").disabled=true;$("#cityRetry").textContent="Kontrol ediliyor…";
  try{await api("/api/city",{city:c});await new Promise(z=>setTimeout(z,3500));fillWx(await api("/api/setup-status"))}catch(e){}
  $("#cityRetry").disabled=false;$("#cityRetry").textContent="Şehri güncelle";
};
$("#finish").onclick=async()=>{
  clearInterval(readyT);
  try{await api("/api/finish",{})}catch(e){}
  show(5);
  const t0=Date.now();
  const probe=()=>{
    if(!ip)return;
    const im=new Image();
    im.onload=()=>{location.href="http://"+ip+"/?hosgeldin=1"};
    im.src="http://"+ip+"/favicon.svg?t="+Date.now();
  };
  setInterval(()=>{probe();if(Date.now()-t0>15000){$("#fallback").hidden=false}},2000);
};

// Sayfa yeniden açılırsa kaldığı yerden devam et
api("/api/setup-status").then(s=>{
  if(s.step=="connecting"||s.step=="checking"){ssid=s.ssid;city=s.city;show(3);pollT=setInterval(poll,1000)}
  else if(s.step=="done"){ssid=s.ssid;city=s.city;ready(s)}
}).catch(()=>{});
</script>
</body></html>
)BL";

// Hem kurulum sayfasının erişim testi hem de panelin sekme simgesi için
const char FAVICON_SVG[] PROGMEM = R"BL(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100"><circle cx="50" cy="50" r="38" fill="none" stroke="#ff8c00" stroke-width="7"/><circle cx="50" cy="50" r="16" fill="#ff8c00"/></svg>)BL";

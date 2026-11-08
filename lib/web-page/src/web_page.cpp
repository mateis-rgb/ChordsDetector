/*
 * web_page.cpp — Page web « Scène Néon », servie depuis la flash (PROGMEM).
 * Rien à téléverser sur LittleFS : la page fait partie du firmware.
 *
 * - Temps réel par WebSocket (/ws) : l'ESP32 pousse l'état
 *   (accord, notes, WiFi, clavier) dès qu'une note change.
 * - Thème clair/sombre automatique via prefers-color-scheme (système).
 * - Mode éco : coupe halos néon, grille, animations et transitions.
 * - Test de réactivité : 20 pings WebSocket, RTT min/moy/max.
 */

#include "web_page.h"

#include <pgmspace.h>

namespace web_page {

const char INDEX_HTML[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover" />
<title>Détecteur d'accords</title>
<style>
:root{
  --bg:#07080c; --text:#eef2f6; --muted:#7d8595;
  --neon1:#22d3ee; --neon2:#f472b6;
  --panel:#0d1017; --panel-border:#1c2230;
  --ok:#34d399; --bad:#f87171;
}
@media (prefers-color-scheme: light){
  :root{
    --bg:#f4f5f7; --text:#0c0e13; --muted:#5d6470;
    --neon1:#0891b2; --neon2:#db2777;
    --panel:#ffffff; --panel-border:#e2e5ea;
    --ok:#059669; --bad:#dc2626;
  }
}
*{box-sizing:border-box;margin:0;padding:0}
body{min-height:100dvh;display:flex;flex-direction:column;
  font-family:system-ui,-apple-system,'Segoe UI',Roboto,sans-serif;color:var(--text);
  background:var(--bg);padding-top:env(safe-area-inset-top);overflow-x:hidden;position:relative}
/* projecteurs + grille décorative */
body::before{content:"";position:fixed;inset:0;z-index:-1;pointer-events:none;
  background:radial-gradient(60% 42% at 50% 8%,rgba(34,211,238,.16),transparent 70%),
             radial-gradient(46% 36% at 82% 92%,rgba(244,114,182,.13),transparent 70%)}
body::after{content:"";position:fixed;inset:0;z-index:-1;pointer-events:none;opacity:.5;
  background-image:linear-gradient(var(--panel-border) 1px,transparent 1px),
                   linear-gradient(90deg,var(--panel-border) 1px,transparent 1px);
  background-size:44px 44px;
  mask-image:radial-gradient(75% 60% at 50% 45%,#000 40%,transparent 100%);
  -webkit-mask-image:radial-gradient(75% 60% at 50% 45%,#000 40%,transparent 100%)}
header{display:flex;flex-wrap:wrap;gap:.6rem;align-items:center;justify-content:space-between;
  padding:1rem 1.25rem;border-bottom:1px solid var(--panel-border)}
.brand{font-weight:800;letter-spacing:-.02em;margin-top:.1rem}
.badge{font-family:ui-monospace,'SF Mono',Menlo,monospace;font-size:.62rem;font-weight:700;
  letter-spacing:.16em;text-transform:uppercase;color:var(--neon1)}
.controls{display:flex;flex-wrap:wrap;gap:.5rem;align-items:center}
.pill,.btn{background:var(--panel);border:1px solid var(--panel-border);color:var(--text);
  font-family:ui-monospace,'SF Mono',Menlo,monospace}
.pill{display:inline-flex;gap:.45rem;align-items:center;padding:.35rem .75rem;
  font-size:.72rem;text-transform:uppercase;letter-spacing:.08em;color:var(--muted);
  border-radius:8px}
.pill .dot{width:.5rem;height:.5rem;border-radius:50%;background:var(--bad);
  box-shadow:0 0 10px var(--bad)}
.pill.on .dot{background:var(--ok);box-shadow:0 0 10px var(--ok)}
.btn{padding:.45rem .85rem;font-size:.75rem;text-transform:uppercase;letter-spacing:.08em;
  border-radius:8px;cursor:pointer;transition:border-color .2s,color .2s,box-shadow .2s}
.btn:hover{border-color:var(--neon1);color:var(--neon1);box-shadow:0 0 18px -4px var(--neon1)}
.btn:active{transform:scale(.96)}
main{flex:1;display:flex;flex-direction:column;align-items:center;justify-content:center;
  gap:1.6rem;padding:1rem;text-align:center}
#chord{font-size:clamp(3.8rem,17vw,10.5rem);font-weight:900;line-height:1;letter-spacing:-.04em;
  color:var(--text);text-shadow:0 0 12px rgba(34,211,238,.55),0 0 46px rgba(34,211,238,.32),
                            0 0 90px rgba(244,114,182,.28)}
#chord .suffix{color:var(--neon1);text-shadow:0 0 14px rgba(34,211,238,.8)}
#chord .slash{color:var(--muted)}
#chord.pop{animation:pop .4s cubic-bezier(.2,.9,.3,1.4)}
@keyframes pop{0%{transform:scale(.88);opacity:.2}100%{transform:scale(1);opacity:1}}
/* mini equalizer décoratif : actif quand des notes jouent */
.eq{display:flex;gap:5px;align-items:flex-end;height:26px;opacity:.15;transition:opacity .3s}
.eq.live{opacity:1}
.eq i{width:5px;border-radius:2px;background:var(--neon1);animation:eq 1.1s ease-in-out infinite}
.eq i:nth-child(2){animation-delay:.15s;background:var(--neon2)}
.eq i:nth-child(3){animation-delay:.3s}
.eq i:nth-child(4){animation-delay:.45s;background:var(--neon2)}
.eq i:nth-child(5){animation-delay:.6s}
@keyframes eq{0%,100%{height:22%}50%{height:100%}}
#notes{display:flex;flex-wrap:wrap;gap:.5rem;justify-content:center;min-height:2.3rem}
.chip{padding:.32rem .8rem;font-size:.88rem;font-variant-numeric:tabular-nums;
  font-family:ui-monospace,'SF Mono',Menlo,monospace;color:var(--text);
  background:var(--panel);border:1px solid var(--panel-border);border-radius:8px;
  animation:rise .35s both}
@keyframes rise{from{transform:translateY(8px);opacity:0}to{transform:none;opacity:1}}
#hint{color:var(--muted);font-size:.95rem;min-height:1.4em}
#latency{font-family:ui-monospace,'SF Mono',Menlo,monospace;font-variant-numeric:tabular-nums;
  font-size:.8rem;color:var(--muted);min-height:1.3em}
/* Mode éco : coupe tout ce qui coûte des ressources */
body.eco *{animation:none!important;transition:none!important;box-shadow:none!important;text-shadow:none!important}
body.eco::before,body.eco::after{display:none}
</style>
</head>
<body>
<header>
  <div><div class="badge">Détecteur d'accords</div><div class="brand">🎹 Accord en cours</div></div>
  <div class="controls">
    <span class="pill" id="pillWifi"><span class="dot"></span>WiFi</span>
    <span class="pill" id="pillClavier"><span class="dot"></span>Clavier</span>
    <button class="btn" id="btnLat">⚡ Réactivité</button>
    <button class="btn" id="btnMode">🌙 Mode éco</button>
    <span class="pill" id="pillTheme">🎨 Auto</span>
  </div>
</header>
<main>
  <div id="chord">—</div>
  <div class="eq" id="eq"><i></i><i></i><i></i><i></i><i></i></div>
  <div id="notes"></div>
  <p id="hint">Jouez une note ou un accord…</p>
  <div id="latency"></div>
</main>
<script>
(function(){
"use strict";
var $=function(i){return document.getElementById(i)};
var chordEl=$("chord"),notesEl=$("notes"),hintEl=$("hint"),latEl=$("latency"),
    eqEl=$("eq"),pillWifi=$("pillWifi"),pillClavier=$("pillClavier"),
    btnLat=$("btnLat"),btnMode=$("btnMode"),pillTheme=$("pillTheme"),
    lastKey=null,ws=null,retry=250;

function setPill(p,ok){p.classList.toggle("on",!!ok)}

/* sépare fondamentale / suffixe / basse pour le néon :
   "Cmaj7/E" -> C + <suffix>maj7</suffix> + <slash>/E</slash> */
function chordHtml(name){
  var m=name.match(/^([A-G]#?)(.*)$/);
  if(!m)return name;
  var rest=m[2].split("/");
  return m[1]
    +(rest[0]?'<span class="suffix">'+rest[0]+"</span>":"")
    +(rest[1]?'<span class="slash">/'+rest[1]+"</span>":"");
}

/* ---------------- Rendu d'un état poussé par l'ESP32 ---------------- */
function render(m){
  var html=(m.count>0&&m.chord)?chordHtml(m.chord):"";
  if(chordEl.innerHTML!==html){
    chordEl.innerHTML=html||"—";
    chordEl.classList.remove("pop");void chordEl.offsetWidth;chordEl.classList.add("pop");
  }
  document.title=m.chord?("♪ "+m.chord):"Détecteur d'accords";
  hintEl.textContent=m.count===0?"Jouez une note ou un accord…":
                     (m.chord?"":"Accord non reconnu");
  eqEl.classList.toggle("live",m.count>0);
  var key=m.notes.map(function(n){return n.m}).join(",");
  if(key!==lastKey){          /* ne retouche le DOM que si les notes changent */
    lastKey=key;notesEl.textContent="";
    m.notes.forEach(function(n,k){
      var s=document.createElement("span");s.className="chip";s.textContent=n.n;
      s.style.animationDelay=(k*45)+"ms";notesEl.appendChild(s);
    });
  }
  setPill(pillWifi,m.wifi);setPill(pillClavier,m.com);
}

/* ---------------- WebSocket temps réel ---------------- */
function connect(){
  ws=new WebSocket((location.protocol==="https:"?"wss://":"ws://")+location.host+"/ws");
  ws.onopen=function(){retry=250;setPill(pillWifi,true)};
  ws.onclose=function(){
    setPill(pillWifi,false);
    setTimeout(connect,retry);retry=Math.min(retry*2,5000);
  };
  ws.onmessage=function(e){
    var m=JSON.parse(e.data);
    if(m.type==="state")render(m);else if(m.type==="pong")onPong(m);
  };
}
connect();

/* ---------------- Test de réactivité (ping/pong WebSocket) ----------------
   20 pings espacés de 70 ms ; l'ESP32 renvoie chaque ping instantanément. */
var testing=false,pings=[],TOTAL=20,t0={};
btnLat.onclick=function(){
  if(testing||!ws||ws.readyState!==1)return;
  testing=true;pings=[];latEl.textContent="Test en cours (20 pings)…";
  for(var i=0;i<TOTAL;i++){
    t0[i]=performance.now();
    (function(id){setTimeout(function(){
      if(ws&&ws.readyState===1)ws.send('{"type":"ping","id":'+id+"}");
    },id*70)})(i);
  }
};
function onPong(m){
  if(!testing||m.id==null)return;
  var id=parseInt(m.id,10);
  if(t0[id]===undefined)return;
  pings.push(performance.now()-t0[id]);t0[id]=undefined;
  if(pings.length>=TOTAL)finish();
}
function finish(){
  testing=false;
  var s=pings.slice().sort(function(a,b){return a-b}),
      min=s[0],max=s[s.length-1],
      avg=pings.reduce(function(a,b){return a+b},0)/pings.length;
  latEl.textContent="RTT : min "+min.toFixed(1)+" / moy "+avg.toFixed(1)
    +" / max "+max.toFixed(1)+" ms — aller simple ≈ "+(avg/2).toFixed(1)+" ms";
}

/* ---------------- Mode éco ---------------- */
function applyMode(eco){
  document.body.classList.toggle("eco",eco);
  btnMode.textContent=eco?"✨ Mode complet":"🌙 Mode éco";
  try{localStorage.setItem("accords-mode",eco?"eco":"rich")}catch(e){}
}
btnMode.onclick=function(){applyMode(!document.body.classList.contains("eco"))};
var stored="rich";try{stored=localStorage.getItem("accords-mode")||"rich"}catch(e){}
applyMode(stored==="eco");

/* ---------------- Badge du thème système (lecture seule) ---------------- */
function theme(){
  var d=window.matchMedia("(prefers-color-scheme: dark)").matches;
  pillTheme.textContent=d?"🌙 Sombre":"☀️ Clair";
}
theme();
window.matchMedia("(prefers-color-scheme: dark)").addEventListener("change",theme);
})();
</script>
</body>
</html>)html";

} // namespace web_page
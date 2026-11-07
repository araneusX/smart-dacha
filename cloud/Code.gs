/**
 * smart-dch Apps Script Web App
 *
 * Deploy: Deploy → New deployment → Web app
 * Execute as: Me
 * Who has access: Anyone
 *
 * Open the spreadsheet → Extensions → Apps Script, paste this file.
 * One sheet per sensor (name = ROM): row 1 notes; row 2 timestamp | t; data from row 3
 *
 * ESP32: POST JSON (type 0/1/2). Browser: GET with type + optional callback= (JSONP).
 * doGet / doPost — имена платформы Apps Script, их нельзя менять.
 */

var CACHE_LIVE = 'live';
var CACHE_TTL = 'ttlUntil';
var HEARTBEAT_MS = 60 * 1000;
var CACHE_SECONDS = 300;

function doPost(e) {
  try {
    var body = {};
    if (e && e.postData && e.postData.contents) {
      body = JSON.parse(e.postData.contents);
    }
    return writeJsonpResponse_(null, handleRequest_(body));
  } catch (err) {
    return writeJsonpResponse_(null, { ok: false, error: String(err) });
  }
}

function doGet(e) {
  try {
    var p = (e && e.parameter) || {};
    var type = p.type !== undefined && p.type !== '' ? Number(p.type) : NaN;
    var body = { type: type };
    return writeJsonpResponse_(p.callback, handleRequest_(body));
  } catch (err) {
    return writeJsonpResponse_(e && e.parameter && e.parameter.callback, {
      ok: false,
      error: String(err)
    });
  }
}

function handleRequest_(body) {
  var type = Number(body.type);
  if (type === 0) {
    return { isLive: isLiveActive_() };
  }
  if (type === 1) {
    appendHistoryRows_(body.sensors);
    if (body.sensors) {
      saveLiveCache_(body.sensors);
    }
    return { ok: true };
  }
  if (type === 2) {
    if (body.sensors) {
      saveLiveCache_(body.sensors);
      return { ok: true };
    }
    return readLiveCache_();
  }
  if (type === 3) {
    extendLiveTtl_();
    return { isLive: true };
  }
  return { ok: false, error: 'bad type' };
}

function isLiveActive_() {
  var raw = CacheService.getScriptCache().get(CACHE_TTL);
  if (!raw) {
    return false;
  }
  return Number(raw) > Date.now();
}

function extendLiveTtl_() {
  var until = Date.now() + HEARTBEAT_MS;
  CacheService.getScriptCache().put(CACHE_TTL, String(until), CACHE_SECONDS);
}

function saveLiveCache_(sensors) {
  var payload = JSON.stringify({ sensors: normalizeSensorRows_(sensors) });
  CacheService.getScriptCache().put(CACHE_LIVE, payload, CACHE_SECONDS);
}

function readLiveCache_() {
  var raw = CacheService.getScriptCache().get(CACHE_LIVE);
  if (!raw) {
    return { sensors: [] };
  }
  return JSON.parse(raw);
}

function floorHistoryTime_(d) {
  var tenMin = 10 * 60 * 1000;
  return new Date(Math.floor(d.getTime() / tenMin) * tenMin);
}

function appendHistoryRows_(sensors) {
  var rows = normalizeSensorRows_(sensors);
  if (!rows.length) {
    return;
  }
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var ts = floorHistoryTime_(new Date());
  for (var i = 0; i < rows.length; i++) {
    var sheet = openSensorSheet_(ss, rows[i].id);
    var start = sheet.getLastRow() + 1;
    if (start < 3) {
      start = 3;
    }
    sheet.getRange(start, 1, 1, 2).setValues([[ts, rows[i].t]]);
    sheet.getRange(start, 1).setNumberFormat('yyyy-mm-dd hh:mm:ss');
    sheet.getRange(start, 2).setNumberFormat('0.0');
  }
}

function normalizeSensorRows_(sensors) {
  if (!sensors || !sensors.length) {
    return [];
  }
  var out = [];
  for (var i = 0; i < sensors.length; i++) {
    var s = sensors[i];
    if (s == null || s.id == null || s.t == null) {
      continue;
    }
    var id = String(s.id);
    var t = Number(s.t);
    if (!id || !isFinite(t)) {
      continue;
    }
    out.push({ id: id, t: Math.round(t * 10) / 10 });
  }
  return out;
}

function makeSheetName_(id) {
  return String(id)
    .replace(/[\\/?*[\]]/g, '_')
    .substring(0, 100);
}

function openSensorSheet_(ss, id) {
  var name = makeSheetName_(id);
  var sheet = ss.getSheetByName(name);
  if (!sheet) {
    sheet = ss.insertSheet(name);
  }
  if (sheet.getLastRow() === 0) {
    sheet.getRange(2, 1, 1, 2).setValues([['timestamp', 't']]);
  }
  return sheet;
}

function writeJsonpResponse_(callback, obj) {
  var text = JSON.stringify(obj);
  if (callback && /^[A-Za-z_][A-Za-z0-9_]*$/.test(callback)) {
    return ContentService.createTextOutput(callback + '(' + text + ')').setMimeType(
      ContentService.MimeType.JAVASCRIPT
    );
  }
  return ContentService.createTextOutput(text).setMimeType(ContentService.MimeType.JSON);
}

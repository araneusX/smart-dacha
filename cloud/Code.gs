/**
 * smart-dch Apps Script Web App
 *
 * Deploy: Deploy → New deployment → Web app
 * Execute as: Me
 * Who has access: Anyone
 *
 * Open the spreadsheet → Extensions → Apps Script, paste this file.
 * Sheet "History" is created automatically: timestamp | id | t
 *
 * ESP32: POST JSON (type 0/1/2). Browser: GET with type + optional callback= (JSONP).
 */

var SHEET_NAME = 'History';
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
    return jsonOutput_(handle_(body));
  } catch (err) {
    return jsonOutput_({ ok: false, error: String(err) });
  }
}

function doGet(e) {
  try {
    var p = (e && e.parameter) || {};
    var type = p.type !== undefined && p.type !== '' ? Number(p.type) : NaN;
    var body = { type: type };
    return maybeJsonp_(p.callback, handle_(body));
  } catch (err) {
    return maybeJsonp_(e && e.parameter && e.parameter.callback, {
      ok: false,
      error: String(err)
    });
  }
}

function handle_(body) {
  var type = Number(body.type);
  if (type === 0) {
    return { isLive: isLive_() };
  }
  if (type === 1) {
    appendHistory_(body.sensors);
    return { ok: true };
  }
  if (type === 2) {
    if (body.sensors) {
      putLive_(body.sensors);
      return { ok: true };
    }
    return getLiveSnapshot_();
  }
  if (type === 3) {
    beat_();
    return { isLive: true };
  }
  return { ok: false, error: 'bad type' };
}

function isLive_() {
  var raw = CacheService.getScriptCache().get(CACHE_TTL);
  if (!raw) {
    return false;
  }
  return Number(raw) > Date.now();
}

function beat_() {
  var until = Date.now() + HEARTBEAT_MS;
  CacheService.getScriptCache().put(CACHE_TTL, String(until), CACHE_SECONDS);
}

function putLive_(sensors) {
  var payload = JSON.stringify({ sensors: normalizeSensors_(sensors) });
  CacheService.getScriptCache().put(CACHE_LIVE, payload, CACHE_SECONDS);
}

function getLiveSnapshot_() {
  var raw = CacheService.getScriptCache().get(CACHE_LIVE);
  if (!raw) {
    return { sensors: [] };
  }
  return JSON.parse(raw);
}

function roundHistoryTime_(d) {
  var tenMin = 10 * 60 * 1000;
  return new Date(Math.floor(d.getTime() / tenMin) * tenMin);
}

function appendHistory_(sensors) {
  var rows = normalizeSensors_(sensors);
  if (!rows.length) {
    return;
  }
  var sheet = historySheet_();
  var ts = roundHistoryTime_(new Date());
  var values = rows.map(function (s) {
    return [ts, s.id, s.t];
  });
  var start = sheet.getLastRow() + 1;
  sheet.getRange(start, 1, values.length, 3).setValues(values);
  sheet.getRange(start, 1, values.length, 1).setNumberFormat('yyyy-mm-dd hh:mm:ss');
}

function normalizeSensors_(sensors) {
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
    out.push({ id: id, t: t });
  }
  return out;
}

function historySheet_() {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheet = ss.getSheetByName(SHEET_NAME);
  if (!sheet) {
    sheet = ss.insertSheet(SHEET_NAME);
  }
  if (sheet.getLastRow() === 0) {
    sheet.getRange(1, 1, 1, 3).setValues([['timestamp', 'id', 't']]);
  }
  return sheet;
}

function jsonOutput_(obj) {
  return ContentService.createTextOutput(JSON.stringify(obj)).setMimeType(
    ContentService.MimeType.JSON
  );
}

function maybeJsonp_(callback, obj) {
  var text = JSON.stringify(obj);
  if (callback && /^[A-Za-z_][A-Za-z0-9_]*$/.test(callback)) {
    return ContentService.createTextOutput(callback + '(' + text + ')').setMimeType(
      ContentService.MimeType.JAVASCRIPT
    );
  }
  return ContentService.createTextOutput(text).setMimeType(ContentService.MimeType.JSON);
}

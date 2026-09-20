/**
 * ESP32 GitHub Cloud Hub - Application Logic
 * Manages state synchronization with GitHub REST API
 */

(function () {
  'use strict';

  // State
  const STORAGE_KEY = 'esp32_github_config_v1';
  const DEFAULT_CONFIG = {
    owner: 'smart-iot-Th',
    repo: 'Server-ESP32',
    branch: 'main',
    path: 'data/state.json',
    token: '',
    interval: 5000
  };

  let config = { ...DEFAULT_CONFIG };

  let currentState = null;
  let currentSha = null;
  let pollIntervalId = null;
  let isCommitting = false;

  // DOM Elements
  const el = {
    statusBadge: document.getElementById('device-status-badge'),
    statusText: document.getElementById('device-status-text'),
    btnRefresh: document.getElementById('btn-refresh'),
    btnOpenSettings: document.getElementById('btn-open-settings'),
    noticeBar: document.getElementById('notice-bar'),
    noticeText: document.getElementById('notice-text'),
    syncIndicator: document.getElementById('sync-indicator'),

    // Glance Bar
    glanceActiveRelays: document.getElementById('glance-active-relays'),
    glanceWifiRssi: document.getElementById('glance-wifi-rssi'),
    glanceTemp: document.getElementById('glance-temp'),

    // Relay Cards (Interactive Tiles)
    relayCard1: document.getElementById('relay-card-1'),
    relayCard2: document.getElementById('relay-card-2'),
    relayCard3: document.getElementById('relay-card-3'),
    relayCard4: document.getElementById('relay-card-4'),

    // Controls
    cmdRelay1: document.getElementById('cmd-relay1'),
    relay1Label: document.getElementById('relay1-state-label'),
    cmdRelay2: document.getElementById('cmd-relay2'),
    relay2Label: document.getElementById('relay2-state-label'),
    cmdRelay3: document.getElementById('cmd-relay3'),
    relay3Label: document.getElementById('relay3-state-label'),
    cmdRelay4: document.getElementById('cmd-relay4'),
    relay4Label: document.getElementById('relay4-state-label'),
    btnTestAllRelays: document.getElementById('btn-test-all-relays'),
    btnTurnAllOn: document.getElementById('btn-turn-all-on'),
    btnTurnAllOff: document.getElementById('btn-turn-all-off'),
    cmdLed: document.getElementById('cmd-led'),
    ledLabel: document.getElementById('led-state-label'),
    btnModeAuto: document.getElementById('btn-mode-auto'),
    btnModeManual: document.getElementById('btn-mode-manual'),
    cmdServerStandby: document.getElementById('cmd-server-standby'),
    standbyStateLabel: document.getElementById('standby-state-label'),
    cmdTargetTemp: document.getElementById('cmd-target-temp'),
    targetTempVal: document.getElementById('target-temp-val'),
    chkSilentMode: document.getElementById('chk-silent-mode'),
    silentModeLabel: document.getElementById('silent-mode-label'),

    // Telemetry
    teleTemp: document.getElementById('tele-temp'),
    tempBar: document.getElementById('temp-bar'),
    teleHumidity: document.getElementById('tele-humidity'),
    humBar: document.getElementById('hum-bar'),
    teleRssi: document.getElementById('tele-rssi'),
    teleRssiQuality: document.getElementById('tele-rssi-quality'),
    teleUptime: document.getElementById('tele-uptime'),
    teleIp: document.getElementById('tele-ip'),
    teleLastSeen: document.getElementById('tele-last-seen'),
    teleSha: document.getElementById('tele-sha'),

    // ESP32 Status Verification Banner
    espBanner: document.getElementById('esp-status-banner'),
    espIcon: document.getElementById('esp-status-icon'),
    espTitle: document.getElementById('esp-status-title'),
    espDesc: document.getElementById('esp-status-desc'),
    btnVerifyEsp: document.getElementById('btn-verify-esp'),

    // Activity Log
    logConsole: document.getElementById('log-console'),
    btnClearLog: document.getElementById('btn-clear-log'),

    // Settings Modal
    modal: document.getElementById('settings-modal'),
    btnCloseModal: document.getElementById('btn-close-modal'),
    cfgOwner: document.getElementById('cfg-owner'),
    cfgRepo: document.getElementById('cfg-repo'),
    cfgBranch: document.getElementById('cfg-branch'),
    cfgPath: document.getElementById('cfg-path'),
    cfgToken: document.getElementById('cfg-token'),
    btnToggleToken: document.getElementById('btn-toggle-token'),
    cfgInterval: document.getElementById('cfg-interval'),
    btnTestConn: document.getElementById('btn-test-connection'),
    btnSaveSettings: document.getElementById('btn-save-settings')
  };

  // =========================================================================
  // Initialization
  // =========================================================================
  function init() {
    loadConfig();
    bindEvents();

    if (config.owner && config.repo) {
      if (config.token && el.noticeBar) el.noticeBar.style.display = 'none';
      log(`กำลังเชื่อมต่อ GitHub API (${config.owner}/${config.repo})...`, 'system');
      fetchState();
      setupPolling();
    } else {
      tryLocalFallback();
    }
  }

  function loadConfig() {
    const saved = localStorage.getItem(STORAGE_KEY);
    if (saved) {
      try {
        const parsed = JSON.parse(saved);
        config = { ...DEFAULT_CONFIG, ...parsed };
      } catch (e) {
        console.error('Failed to parse saved config', e);
      }
    }

    // Auto-fix: แก้ไขและลบชื่อโฟลเดอร์เครื่อง "Server ESP32/" ที่อาจปนเปื้อนมา
    if (config.path) {
      config.path = config.path.replace(/^Server[\s_-]*ESP32\/?/i, '').trim();
      if (!config.path || config.path.includes('Server ESP32')) {
        config.path = 'data/state.json';
      }
    } else {
      config.path = 'data/state.json';
    }

    // Auto-fix: ชื่อ repo ต้องเป็น Server-ESP32
    if (!config.repo || config.repo === 'Server ESP32' || config.repo.includes('Server ESP32')) {
      config.repo = 'Server-ESP32';
    }

    // Auto-fix: owner ต้องเป็น smart-iot-Th
    if (!config.owner || config.owner === '67125530108-lang') {
      config.owner = 'smart-iot-Th';
    }

    // บันทึกค่าที่ถูกแก้ไขถูกต้องกลับลง localStorage ทันที
    localStorage.setItem(STORAGE_KEY, JSON.stringify(config));

    // Fill modal fields with detected/saved values or defaults
    el.cfgOwner.value = config.owner;
    el.cfgRepo.value = config.repo;
    el.cfgBranch.value = config.branch || 'main';
    el.cfgPath.value = config.path;
    el.cfgToken.value = config.token || '';
    el.cfgInterval.value = config.interval !== undefined ? config.interval : '5000';
  }

  function saveConfig() {
    config.owner = el.cfgOwner.value.trim() || DEFAULT_CONFIG.owner;
    let repoVal = el.cfgRepo.value.trim() || DEFAULT_CONFIG.repo;
    if (repoVal === 'Server ESP32' || repoVal.includes('Server ESP32')) repoVal = 'Server-ESP32';
    config.repo = repoVal;
    config.branch = el.cfgBranch.value.trim() || 'main';

    let pathVal = el.cfgPath.value.trim() || 'data/state.json';
    pathVal = pathVal.replace(/^Server[\s_-]*ESP32\/?/i, '').trim();
    if (!pathVal || pathVal.includes('Server ESP32')) pathVal = 'data/state.json';
    config.path = pathVal;

    config.token = el.cfgToken.value.trim();
    config.interval = parseInt(el.cfgInterval.value, 10) || 5000;

    localStorage.setItem(STORAGE_KEY, JSON.stringify(config));
    log('บันทึกการตั้งค่า GitHub สำเร็จ', 'success');

    el.modal.classList.remove('active');
    if (config.token && el.noticeBar) el.noticeBar.style.display = 'none';

    setupPolling();
    fetchState();
  }

  // =========================================================================
  // GitHub API Communications
  // =========================================================================
  function getApiHeaders() {
    const headers = {
      'Accept': 'application/vnd.github+json',
      'X-GitHub-Api-Version': '2022-11-28'
    };
    if (config.token) {
      headers['Authorization'] = `Bearer ${config.token}`;
    }
    return headers;
  }

  async function fetchState() {
    if (!config.owner || !config.repo) {
      return;
    }

    el.btnRefresh.classList.add('spinning');
    el.syncIndicator.textContent = 'กำลังดึงข้อมูล...';

    const url = `https://api.github.com/repos/${config.owner}/${config.repo}/contents/${config.path}?ref=${config.branch}&_t=${Date.now()}`;

    try {
      const res = await fetch(url, {
        headers: getApiHeaders()
      });

      if (!res.ok) {
        if (res.status === 401 || res.status === 403) {
          throw new Error(`การยืนยันตัวตนล้มเหลว (${res.status}) กรุณาตรวจสอบสิทธิ์ GitHub Token ในหน้าต่างตั้งค่า`);
        } else if (res.status === 404) {
          // หากพาธผิด ให้แก้ไขเป็น data/state.json ให้อัตโนมัติทันที
          if (config.path !== 'data/state.json' || config.repo !== 'Server-ESP32') {
            log(`ตรวจพบพาธไฟล์ ${config.path} ไม่ถูกต้อง กำลังปรับเป็น data/state.json ให้อัตโนมัติ...`, 'warn');
            config.path = 'data/state.json';
            config.repo = 'Server-ESP32';
            localStorage.setItem(STORAGE_KEY, JSON.stringify(config));
            if (el.cfgPath) el.cfgPath.value = 'data/state.json';
            if (el.cfgRepo) el.cfgRepo.value = 'Server-ESP32';
            return await fetchState();
          }
          throw new Error(`ไม่พบไฟล์ ${config.path} บน Repository ${config.owner}/${config.repo}`);
        } else {
          throw new Error(`GitHub API ตอบกลับสถานะ ${res.status}: ${res.statusText}`);
        }
      }

      const data = await res.json();
      currentSha = data.sha;

      // Decode base64 content
      const decodedStr = decodeBase64Utf8(data.content);
      const parsed = JSON.parse(decodedStr);

      currentState = parsed;
      updateUIWithState(currentState);
      el.syncIndicator.textContent = `ซิงค์แล้ว (${new Date().toLocaleTimeString('th-TH')})`;
      log(`ดึงข้อมูลสำเร็จ (SHA: ${currentSha.substring(0, 7)})`, 'system');

    } catch (err) {
      console.error(err);
      el.syncIndicator.textContent = 'เกิดข้อผิดพลาดในการดึงข้อมูล';
      log(`เกิดข้อผิดพลาด: ${err.message}`, 'error');
    } finally {
      el.btnRefresh.classList.remove('spinning');
    }
  }

  let autoCommitTimer = null;
  function scheduleAutoCommit(customCommands = null) {
    if (el.syncIndicator) {
      el.syncIndicator.textContent = '⚡ กำลังส่งคำสั่ง...';
      el.syncIndicator.style.color = '#38bdf8';
    }
    clearTimeout(autoCommitTimer);
    autoCommitTimer = setTimeout(async () => {
      await commitCommands(customCommands);
    }, 120);
  }

  async function commitCommands(customCommands = null) {
    if (!config.owner || !config.repo || !config.token) {
      openModal();
      log('กรุณาตั้งค่า GitHub Token ก่อนทำการสั่งงาน', 'warn');
      return;
    }

    if (isCommitting) {
      // หากกำลังส่งอยู่ ให้ส่งคำสั่งล่าสุดตามไปอีกรอบ
      clearTimeout(autoCommitTimer);
      autoCommitTimer = setTimeout(() => commitCommands(customCommands), 300);
      return;
    }
    isCommitting = true;

    if (el.syncIndicator) {
      el.syncIndicator.textContent = '⚡ กำลังส่งคำสั่งเรียลไทม์...';
      el.syncIndicator.style.color = '#38bdf8';
    }

    try {
      // 1. Ensure we have the latest SHA
      if (!currentSha) {
        await fetchState();
      }

      // 2. Prepare payload
      const isStandby = el.cmdServerStandby ? el.cmdServerStandby.checked : false;
      const mode = isStandby ? 'standby' : (el.btnModeAuto.classList.contains('active') ? 'auto' : 'manual');
      const commands = customCommands || {
        relay1: isStandby ? false : el.cmdRelay1.checked,
        relay2: isStandby ? false : el.cmdRelay2.checked,
        relay3: isStandby ? false : (el.cmdRelay3 ? el.cmdRelay3.checked : false),
        relay4: isStandby ? false : (el.cmdRelay4 ? el.cmdRelay4.checked : false),
        led: isStandby ? false : el.cmdLed.checked,
        test_relays: false,
        mode: mode,
        target_temp: parseFloat(el.cmdTargetTemp.value)
      };

      const nowIso = new Date().toISOString();
      const updatedState = {
        commands: commands,
        telemetry: currentState ? currentState.telemetry : {
          temperature: 0,
          humidity: 0,
          rssi: 0,
          uptime_sec: 0,
          last_seen: nowIso,
          ip_address: "unknown"
        },
        meta: {
          version: "1.0.0",
          updated_by: "web_dashboard",
          updated_at: nowIso
        }
      };

      const jsonStr = JSON.stringify(updatedState, null, 2);
      const encodedContent = encodeBase64Utf8(jsonStr);
      const commitUrl = `https://api.github.com/repos/${config.owner}/${config.repo}/contents/${config.path}`;
      const commitMsg = isStandby
        ? 'Dashboard: Put server to Standby (Power & Quota Saving)'
        : (commands.test_relays
            ? 'Dashboard: Run Relay Self-Test Sequence (1-4)'
            : `Dashboard: Quick Toggle (R1:${commands.relay1 ? 'ON' : 'OFF'}, R2:${commands.relay2 ? 'ON' : 'OFF'}, R3:${commands.relay3 ? 'ON' : 'OFF'}, R4:${commands.relay4 ? 'ON' : 'OFF'}, LED:${commands.led ? 'ON' : 'OFF'})`);

      const isSilent = el.chkSilentMode ? el.chkSilentMode.checked : true;
      const finalCommitMsg = commitMsg + (isSilent ? ' [skip ci] [silent] [no-notify]' : '');

      const payload = {
        message: finalCommitMsg,
        content: encodedContent,
        sha: currentSha,
        branch: config.branch
      };

      if (isSilent) {
        payload.committer = {
          name: "github-actions[bot]",
          email: "41898282+github-actions[bot]@users.noreply.github.com"
        };
        payload.author = {
          name: "github-actions[bot]",
          email: "41898282+github-actions[bot]@users.noreply.github.com"
        };
      }

      const res = await fetch(commitUrl, {
        method: 'PUT',
        headers: getApiHeaders(),
        body: JSON.stringify(payload)
      });

      if (!res.ok) {
        if (res.status === 409) {
          throw new Error('เกิดสถานะ Conflict (SHA ไม่อัปเดต) กำลังดึงข้อมูลล่าสุด...');
        }
        const errData = await res.json();
        throw new Error(errData.message || `HTTP ${res.status}`);
      }

      const resData = await res.json();
      currentSha = resData.content.sha;
      currentState = updatedState;

      const timeStr = new Date().toLocaleTimeString('th-TH');
      log(`สั่งการสำเร็จ: ${commitMsg} (SHA: ${currentSha.substring(0, 7)})`, 'success');
      if (el.syncIndicator) {
        el.syncIndicator.textContent = `🟢 คำสั่งทำงานแล้ว (${timeStr})`;
        el.syncIndicator.style.color = '#34d399';
      }

      if (isStandby) {
        if (pollIntervalId) {
          clearInterval(pollIntervalId);
          pollIntervalId = null;
        }
        el.syncIndicator.textContent = '⏸️ เซิร์ฟเวอร์พักการทำงาน (Standby)';
        el.statusBadge.className = 'status-badge standby';
        el.statusText.textContent = 'พักระบบ (Standby)';
      } else {
        setupPolling();
      }

    } catch (err) {
      console.error(err);
      log(`ล้มเหลวในการส่งคำสั่ง: ${err.message}`, 'error');
      if (el.syncIndicator) {
        el.syncIndicator.textContent = '❌ ส่งคำสั่งไม่สำเร็จ';
        el.syncIndicator.style.color = '#f87171';
      }
      // Retry fetching state to re-sync SHA
      await fetchState();
    } finally {
      isCommitting = false;
    }
  }

  // =========================================================================
  // Local Demo Fallback (If accessed locally before config)
  // =========================================================================
  async function tryLocalFallback() {
    try {
      let res = await fetch('data/state.json').catch(() => null);
      if (!res || !res.ok) {
        res = await fetch('../data/state.json').catch(() => null);
      }
      if (res && res.ok) {
        const data = await res.json();
        currentState = data;
        updateUIWithState(data);
        log('โหลดข้อมูลตัวอย่างจากไฟล์ local data/state.json เรียบร้อย', 'system');
      }
    } catch (e) {
      // Silently ignore if running on non-server file protocol
    }
  }

  // =========================================================================
  // UI State Updates
  // =========================================================================
  function updateUIWithState(state) {
    if (!state) return;

    // 1. Commands
    if (state.commands) {
      const { relay1, relay2, relay3, relay4, led, mode, target_temp } = state.commands;

      el.cmdRelay1.checked = !!relay1;
      el.relay1Label.textContent = relay1 ? 'ON' : 'OFF';
      el.relay1Label.className = `state-text ${relay1 ? 'active' : ''}`;

      el.cmdRelay2.checked = !!relay2;
      el.relay2Label.textContent = relay2 ? 'ON' : 'OFF';
      el.relay2Label.className = `state-text ${relay2 ? 'active' : ''}`;

      if (el.cmdRelay3 && el.relay3Label) {
        el.cmdRelay3.checked = !!relay3;
        el.relay3Label.textContent = relay3 ? 'ON' : 'OFF';
        el.relay3Label.className = `state-text ${relay3 ? 'active' : ''}`;
      }

      if (el.cmdRelay4 && el.relay4Label) {
        el.cmdRelay4.checked = !!relay4;
        el.relay4Label.textContent = relay4 ? 'ON' : 'OFF';
        el.relay4Label.className = `state-text ${relay4 ? 'active' : ''}`;
      }

      el.cmdLed.checked = !!led;
      el.ledLabel.textContent = led ? 'ON' : 'OFF';
      el.ledLabel.className = `state-text ${led ? 'active' : ''}`;

      if (mode === 'manual') {
        el.btnModeManual.classList.add('active');
        el.btnModeAuto.classList.remove('active');
      } else {
        el.btnModeAuto.classList.add('active');
        el.btnModeManual.classList.remove('active');
      }

      const isStandby = mode === 'standby';
      if (el.cmdServerStandby) {
        el.cmdServerStandby.checked = isStandby;
      }
      if (el.standbyStateLabel) {
        el.standbyStateLabel.textContent = isStandby ? 'พักระบบ (Standby)' : 'ทำงานปกติ';
        el.standbyStateLabel.className = `state-text ${isStandby ? 'standby' : 'active'}`;
      }
      if (isStandby) {
        if (pollIntervalId) {
          clearInterval(pollIntervalId);
          pollIntervalId = null;
        }
        el.syncIndicator.textContent = '⏸️ เซิร์ฟเวอร์พักการทำงาน (Standby)';
        el.statusBadge.className = 'status-badge standby';
        el.statusText.textContent = 'พักระบบ (Standby)';
      }

      if (target_temp !== undefined) {
        el.cmdTargetTemp.value = target_temp;
        el.targetTempVal.textContent = `${parseFloat(target_temp).toFixed(1)} °C`;
      }

      // อัปเดตสถานะการแสดงผลและแสงนีออนของ Relay Tiles ทันที
      updateRelayTilesVisuals();
    }

    // 2. Telemetry & ESP32 Live Verification
    // ตรวจสอบจาก telemetry.last_seen เป็นหลัก (ไม่ตัดการเชื่อมต่อเมื่อ Dashboard ทำการบันทึกคำสั่ง)
    const hasLastSeen = !!(state.telemetry && state.telemetry.last_seen);

    if (hasLastSeen) {
      const { temperature, humidity, rssi, uptime_sec, last_seen, ip_address } = state.telemetry;
      const lastSeenDate = new Date(last_seen);
      el.teleLastSeen.textContent = lastSeenDate.toLocaleString('th-TH');

      const diffSeconds = (Date.now() - lastSeenDate.getTime()) / 1000;

      // ESP32 ส่ง Telemetry ทุกๆ 45 วินาที ให้ Threshold เป็น 120 วินาทีเพื่อความเสถียร ไม่หลุดบ่อย
      if (diffSeconds <= 120) {
        setDeviceOnline(true, `ESP32 ออนไลน์ (${Math.max(1, Math.round(diffSeconds))} วิที่แล้ว)`);
        setEspBanner('online', '🟢', 'บอร์ด ESP32 ออนไลน์และส่งข้อมูลปกติ',
          `เชื่อมต่อกับ GitHub สำเร็จ ล่าสุดเมื่อ ${Math.max(1, Math.round(diffSeconds))} วินาทีที่แล้ว (${lastSeenDate.toLocaleTimeString('th-TH')})`);
      } else {
        setDeviceOnline(false, `ESP32 ออฟไลน์ (${formatTimeDiff(diffSeconds)})`);
        setEspBanner('offline', '🔴', 'บอร์ด ESP32 ออฟไลน์ (ขาดการติดต่อ)',
          `ขาดการติดต่อจากบอร์ดมาแล้ว ${formatTimeDiff(diffSeconds)} (รายงานตัวล่าสุด: ${lastSeenDate.toLocaleTimeString('th-TH')})`);
      }

      // แสดงค่าเซนเซอร์จริงจากบอร์ด
      if (temperature !== undefined) {
        const tempFormatted = parseFloat(temperature).toFixed(1);
        el.teleTemp.textContent = tempFormatted;
        const tempPct = Math.min(Math.max(((temperature - 10) / 40) * 100, 0), 100);
        el.tempBar.style.width = `${tempPct}%`;
        if (el.glanceTemp) {
          el.glanceTemp.textContent = `${tempFormatted} °C`;
        }
      }

      if (humidity !== undefined) {
        el.teleHumidity.textContent = parseFloat(humidity).toFixed(1);
        el.humBar.style.width = `${Math.min(Math.max(humidity, 0), 100)}%`;
      }

      if (rssi !== undefined) {
        el.teleRssi.textContent = rssi;
        let qualityText = 'สัญญาณยอดเยี่ยม';
        if (rssi < -80) qualityText = 'สัญญาณอ่อนมาก';
        else if (rssi < -70) qualityText = 'สัญญาณปานกลาง';
        else if (rssi < -60) qualityText = 'สัญญาณดี';
        el.teleRssiQuality.textContent = qualityText;
        if (el.glanceWifiRssi) {
          el.glanceWifiRssi.textContent = `${rssi} dBm`;
        }
      }

      if (uptime_sec !== undefined) {
        el.teleUptime.textContent = formatUptime(uptime_sec);
      }

      if (ip_address) {
        el.teleIp.textContent = `IP: ${ip_address}`;
      }
    } else {
      // ยังไม่เคยมีข้อมูลจากบอร์ดจริง (หรือข้อมูลมาจาก mock setup)
      setDeviceOnline(false, 'รอการเชื่อมต่อบอร์ด ESP32');
      setEspBanner('waiting', '⏳', 'รอ ESP32 เชื่อมต่อและส่งข้อมูลครั้งแรก',
        'ระบบไม่พบข้อมูลที่ส่งมาจากบอร์ด ESP32 จริง ค่าเซนเซอร์จะเริ่มแสดงเมื่อบอร์ดเริ่มทำงานจริงเท่านั้น เพื่อป้องกันการแสดงค่าสุ่ม/มั่ว');

      el.teleTemp.textContent = '--.-';
      el.tempBar.style.width = '0%';
      el.teleHumidity.textContent = '--.-';
      el.humBar.style.width = '0%';
      el.teleRssi.textContent = '--';
      el.teleRssiQuality.textContent = 'ยังไม่พบสัญญาณจากบอร์ด';
      el.teleUptime.textContent = '--';
      el.teleIp.textContent = 'IP: รอเชื่อมต่อ';
      el.teleLastSeen.textContent = 'ยังไม่เคยเชื่อมต่อจริง';
      if (el.glanceTemp) el.glanceTemp.textContent = '--.- °C';
      if (el.glanceWifiRssi) el.glanceWifiRssi.textContent = '-- dBm';
    }

    if (currentSha) {
      el.teleSha.textContent = currentSha;
    }
  }

  function setDeviceOnline(isOnline, reason) {
    if (isOnline) {
      el.statusBadge.className = 'status-badge online';
      el.statusText.textContent = reason || 'ESP32 ออนไลน์';
    } else {
      el.statusBadge.className = 'status-badge offline';
      el.statusText.textContent = reason || 'บอร์ดออฟไลน์';
    }
  }

  function setEspBanner(status, icon, title, desc) {
    if (!el.espBanner) return;
    el.espBanner.className = `esp-status-banner ${status}`;
    if (el.espIcon) el.espIcon.textContent = icon;
    if (el.espTitle) el.espTitle.textContent = title;
    if (el.espDesc) el.espDesc.textContent = desc;
  }

  // Helper: อัปเดตกราฟิกและแสงนีออนของ Relay Tiles และ Glance Bar
  function updateRelayTilesVisuals() {
    let activeCount = 0;
    [1, 2, 3, 4].forEach(i => {
      const chk = el[`cmdRelay${i}`];
      const card = el[`relayCard${i}`];
      if (chk && card) {
        if (chk.checked) {
          card.classList.add('is-on');
          activeCount++;
        } else {
          card.classList.remove('is-on');
        }
      }
    });
    if (el.glanceActiveRelays) {
      el.glanceActiveRelays.textContent = `${activeCount} / 4 ช่อง`;
    }
  }

  // Helper: ตั้งค่ารีเลย์ทุกช่องพร้อมกัน
  function setAllRelays(state) {
    [1, 2, 3, 4].forEach(i => {
      const chk = el[`cmdRelay${i}`];
      const lbl = el[`relay${i}Label`];
      if (chk && lbl) {
        chk.checked = state;
        lbl.textContent = state ? 'ON' : 'OFF';
        lbl.className = `state-text ${state ? 'active' : ''}`;
      }
    });
    updateRelayTilesVisuals();
  }

  // =========================================================================
  // Event Listeners
  // =========================================================================
  function bindEvents() {
    // Switch Toggles (ส่งคำสั่งทันทีแบบเรียลไทม์)
    el.cmdRelay1.addEventListener('change', () => {
      const on = el.cmdRelay1.checked;
      el.relay1Label.textContent = on ? 'ON' : 'OFF';
      el.relay1Label.className = `state-text ${on ? 'active' : ''}`;
      updateRelayTilesVisuals();
      scheduleAutoCommit();
    });

    el.cmdRelay2.addEventListener('change', () => {
      const on = el.cmdRelay2.checked;
      el.relay2Label.textContent = on ? 'ON' : 'OFF';
      el.relay2Label.className = `state-text ${on ? 'active' : ''}`;
      updateRelayTilesVisuals();
      scheduleAutoCommit();
    });

    if (el.cmdRelay3) {
      el.cmdRelay3.addEventListener('change', () => {
        const on = el.cmdRelay3.checked;
        el.relay3Label.textContent = on ? 'ON' : 'OFF';
        el.relay3Label.className = `state-text ${on ? 'active' : ''}`;
        updateRelayTilesVisuals();
        scheduleAutoCommit();
      });
    }

    if (el.cmdRelay4) {
      el.cmdRelay4.addEventListener('change', () => {
        const on = el.cmdRelay4.checked;
        el.relay4Label.textContent = on ? 'ON' : 'OFF';
        el.relay4Label.className = `state-text ${on ? 'active' : ''}`;
        updateRelayTilesVisuals();
        scheduleAutoCommit();
      });
    }

    // Quick Actions
    if (el.btnTurnAllOn) {
      el.btnTurnAllOn.addEventListener('click', () => {
        setAllRelays(true);
        scheduleAutoCommit();
        log('สั่งเปิดรีเลย์ทั้ง 4 ช่องทันที', 'normal');
      });
    }

    if (el.btnTurnAllOff) {
      el.btnTurnAllOff.addEventListener('click', () => {
        setAllRelays(false);
        scheduleAutoCommit();
        log('สั่งปิดรีเลย์ทั้ง 4 ช่องทันที', 'normal');
      });
    }

    if (el.btnTestAllRelays) {
      el.btnTestAllRelays.addEventListener('click', async () => {
        log('🧪 เริ่มสั่งทดสอบวงจรรีเลย์ 4 ช่อง (Self-Test Sequence)...', 'system');
        const isStandby = el.cmdServerStandby ? el.cmdServerStandby.checked : false;
        const mode = isStandby ? 'standby' : (el.btnModeAuto.classList.contains('active') ? 'auto' : 'manual');
        const testCommands = {
          relay1: false,
          relay2: false,
          relay3: false,
          relay4: false,
          led: el.cmdLed.checked,
          test_relays: true,
          mode: mode,
          target_temp: parseFloat(el.cmdTargetTemp.value)
        };
        await commitCommands(testCommands);
      });
    }

    el.cmdLed.addEventListener('change', () => {
      const on = el.cmdLed.checked;
      el.ledLabel.textContent = on ? 'ON' : 'OFF';
      el.ledLabel.className = `state-text ${on ? 'active' : ''}`;
      scheduleAutoCommit();
    });

    // Mode Buttons
    el.btnModeAuto.addEventListener('click', () => {
      el.btnModeAuto.classList.add('active');
      el.btnModeManual.classList.remove('active');
      scheduleAutoCommit();
    });

    el.btnModeManual.addEventListener('click', () => {
      el.btnModeManual.classList.add('active');
      el.btnModeAuto.classList.remove('active');
      scheduleAutoCommit();
    });

    // Server Standby Switch
    if (el.cmdServerStandby) {
      el.cmdServerStandby.addEventListener('change', () => {
        const isStandby = el.cmdServerStandby.checked;
        if (el.standbyStateLabel) {
          el.standbyStateLabel.textContent = isStandby ? 'พักระบบ (Standby)' : 'ทำงานปกติ';
          el.standbyStateLabel.className = `state-text ${isStandby ? 'standby' : 'active'}`;
        }
        if (isStandby) {
          setAllRelays(false);
          el.cmdLed.checked = false;
          el.ledLabel.textContent = 'OFF';
          el.ledLabel.className = 'state-text';

          if (pollIntervalId) {
            clearInterval(pollIntervalId);
            pollIntervalId = null;
          }
          el.syncIndicator.textContent = '⏸️ เซิร์ฟเวอร์พักการทำงาน (Standby)';
          log('เปิดโหมดพักเซิร์ฟเวอร์ (Standby)', 'warn');
        } else {
          setupPolling();
          el.syncIndicator.textContent = '🟢 เซิร์ฟเวอร์ทำงานปกติ';
          log('ปิดโหมดพักเซิร์ฟเวอร์: กลับสู่การทำงานปกติ', 'system');
        }
        scheduleAutoCommit();
      });
    }

    // Target Temp Slider
    el.cmdTargetTemp.addEventListener('change', (e) => {
      el.targetTempVal.textContent = `${parseFloat(e.target.value).toFixed(1)} °C`;
      scheduleAutoCommit();
    });
    el.cmdTargetTemp.addEventListener('input', (e) => {
      el.targetTempVal.textContent = `${parseFloat(e.target.value).toFixed(1)} °C`;
    });

    // Silent Mode Toggle Listener
    if (el.chkSilentMode) {
      el.chkSilentMode.addEventListener('change', () => {
        const isSilent = el.chkSilentMode.checked;
        if (el.silentModeLabel) {
          el.silentModeLabel.textContent = isSilent
            ? '🔕 ส่งแบบเงียบ (Silent Mode - ไม่แจ้งเตือนเข้าอีเมล)'
            : '🔔 ส่งพร้อมแจ้งเตือนปกติ (อาจมีอีเมลจาก GitHub)';
        }
        log(isSilent ? 'เปิดโหมดส่งแบบเงียบ (ไม่ส่งอีเมลแจ้งเตือน)' : 'ปิดโหมดส่งแบบเงียบ (จะส่งแจ้งเตือนตามปกติ)', 'normal');
      });
    }

    // Header Refresh Action
    el.btnRefresh.addEventListener('click', () => {
      fetchState();
    });

    // ESP32 Live Verification Button
    if (el.btnVerifyEsp) {
      el.btnVerifyEsp.addEventListener('click', async () => {
        const svg = el.btnVerifyEsp.querySelector('svg');
        if (svg) svg.classList.add('spinning');
        log('กำลังตรวจสอบสถานะการออนไลน์ของบอร์ด ESP32 จาก GitHub API...', 'system');
        await fetchState();
        setTimeout(() => {
          if (svg) svg.classList.remove('spinning');
        }, 600);
      });
    }

    // Settings Modal
    el.btnOpenSettings.addEventListener('click', openModal);
    el.btnCloseModal.addEventListener('click', closeModal);
    el.modal.addEventListener('click', (e) => {
      if (e.target === el.modal) closeModal();
    });

    el.btnToggleToken.addEventListener('click', () => {
      const type = el.cfgToken.type === 'password' ? 'text' : 'password';
      el.cfgToken.type = type;
      el.btnToggleToken.textContent = type === 'password' ? '👁️' : '🔒';
    });

    el.btnSaveSettings.addEventListener('click', saveConfig);

    el.btnTestConn.addEventListener('click', async () => {
      const owner = el.cfgOwner.value.trim();
      const repo = el.cfgRepo.value.trim();
      const branch = el.cfgBranch.value.trim() || 'main';
      const token = el.cfgToken.value.trim();

      if (!owner || !repo) {
        alert('กรุณากรอก Repository Owner และ Name ให้ครบถ้วน');
        return;
      }

      el.btnTestConn.disabled = true;
      el.btnTestConn.innerHTML = `<span>กำลังตรวจสอบ...</span>`;

      try {
        const testHeaders = {
          'Accept': 'application/vnd.github+json',
          'X-GitHub-Api-Version': '2022-11-28'
        };
        if (token) testHeaders['Authorization'] = `Bearer ${token}`;

        const res = await fetch(`https://api.github.com/repos/${owner}/${repo}`, {
          headers: testHeaders
        });

        if (res.ok) {
          const repoData = await res.json();
          alert(`เชื่อมต่อ GitHub Repository สำเร็จ!\nชื่อ: ${repoData.full_name}\nความเป็นส่วนตัว: ${repoData.private ? 'Private (ส่วนตัว)' : 'Public (สาธารณะ)'}`);
        } else {
          alert(`เชื่อมต่อไม่สำเร็จ: HTTP ${res.status}\nกรุณาตรวจสอบชื่อ Repository หรือ Token`);
        }
      } catch (err) {
        alert(`เกิดข้อผิดพลาดในการเชื่อมต่อ: ${err.message}`);
      } finally {
        el.btnTestConn.disabled = false;
        el.btnTestConn.innerHTML = `<span>ทดสอบการเชื่อมต่อ</span>`;
      }
    });

    // Clear Log
    el.btnClearLog.addEventListener('click', () => {
      el.logConsole.innerHTML = '';
      log('ล้างบันทึกเรียบร้อย', 'system');
    });
  }

  function markUncommitted() {
    el.syncIndicator.textContent = '⚠️ มีคำสั่งที่ยังไม่ได้บันทึก (กดปุ่มบันทึกด้านล่าง)';
  }

  function openModal() {
    el.modal.classList.add('active');
  }

  function closeModal() {
    el.modal.classList.remove('active');
  }

  function setupPolling() {
    if (pollIntervalId) {
      clearInterval(pollIntervalId);
      pollIntervalId = null;
    }

    const interval = parseInt(config.interval, 10);
    if (interval > 0) {
      pollIntervalId = setInterval(() => {
        // Only poll if not currently committing
        if (!isCommitting) {
          fetchState();
        }
      }, interval);
    }
  }

  // =========================================================================
  // Helpers
  // =========================================================================
  function log(message, type = 'normal') {
    const time = new Date().toLocaleTimeString('th-TH');
    const entry = document.createElement('div');
    entry.className = `log-entry ${type}`;
    entry.textContent = `[${time}] ${message}`;
    el.logConsole.appendChild(entry);
    el.logConsole.scrollTop = el.logConsole.scrollHeight;
  }

  function formatUptime(seconds) {
    const s = parseInt(seconds, 10) || 0;
    const hrs = Math.floor(s / 3600);
    const mins = Math.floor((s % 3600) / 60);
    const secs = s % 60;
    return `${hrs}h ${mins.toString().padStart(2, '0')}m ${secs.toString().padStart(2, '0')}s`;
  }

  function formatTimeDiff(seconds) {
    if (seconds < 60) return `${Math.floor(seconds)} วินาที`;
    if (seconds < 3600) return `${Math.floor(seconds / 60)} นาที`;
    return `${Math.floor(seconds / 3600)} ชั่วโมง`;
  }

  // Base64 UTF-8 Encoding/Decoding for GitHub API
  function encodeBase64Utf8(str) {
    return btoa(unescape(encodeURIComponent(str)));
  }

  function decodeBase64Utf8(base64) {
    // Remove newline characters from GitHub base64
    const cleanBase64 = base64.replace(/\s/g, '');
    return decodeURIComponent(escape(atob(cleanBase64)));
  }

  // Start app
  init();

})();

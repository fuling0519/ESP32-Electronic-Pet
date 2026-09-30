import 'dart:async';
import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'data/ble/ble_snapshot_adapter.dart';
import 'domain/models/web_pet.dart';

void main() => runApp(const Esp32PetApp());

class Esp32PetApp extends StatelessWidget {
  const Esp32PetApp({super.key});

  @override
  Widget build(BuildContext context) => MaterialApp(
        title: 'ESP32 Pet',
        debugShowCheckedModeBanner: false,
        theme: ThemeData(
          colorScheme: ColorScheme.fromSeed(seedColor: const Color(0xffeab308)),
          useMaterial3: true,
          scaffoldBackgroundColor: const Color(0xfffffbeb),
        ),
        home: const PetHomePage(),
      );
}

class PetHomePage extends StatefulWidget {
  const PetHomePage({super.key});
  @override
  State<PetHomePage> createState() => _PetHomePageState();
}

class _PetHomePageState extends State<PetHomePage> {
  static final Guid _serviceUuid = Guid('7d2a0001-8b7c-4f3a-9c2d-1e5f6a7b8c90');
  static final Guid _commandUuid = Guid('7d2a0002-8b7c-4f3a-9c2d-1e5f6a7b8c90');
  static final Guid _eventUuid = Guid('7d2a0003-8b7c-4f3a-9c2d-1e5f6a7b8c90');
  static const int _maxMessageBytes = 1024;
  static const int _fragmentBytes = 16;

  BluetoothDevice? _device;
  BluetoothCharacteristic? _command;
  StreamSubscription<List<ScanResult>>? _scanSubscription;
  StreamSubscription<List<int>>? _eventSubscription;
  StreamSubscription<BluetoothConnectionState>? _connectionSubscription;
  bool _scanning = false, _connecting = false, _waitingForData = false;
  String _status = '尚未連線';
  String? _deviceId;
  WebPetSnapshot? _pet;
  DateTime? _lastReceived;
  int _nextRequestId = 0, _nextMessageId = 0;
  final Map<String, ScanResult> _found = {};
  final Map<int, Completer<Map<String, dynamic>>> _pending = {};
  final Map<int, _IncomingMessage> _incoming = {};

  @override
  void dispose() {
    _scanSubscription?.cancel();
    _eventSubscription?.cancel();
    _connectionSubscription?.cancel();
    for (final pending in _pending.values) {
      if (!pending.isCompleted) pending.completeError(StateError('頁面已關閉'));
    }
    super.dispose();
  }

  Future<void> _scan() async {
    if (_scanning || _connecting) return;
    try {
      if (!await FlutterBluePlus.isSupported) {
        _message('此平台或裝置不支援 BLE');
        return;
      }
      await _scanSubscription?.cancel();
      setState(() {
        _found.clear();
        _scanning = true;
        _status = '正在搜尋裝置';
      });
      _scanSubscription = FlutterBluePlus.onScanResults.listen((results) {
        if (!mounted) return;
        setState(() {
          for (final result in results) {
            final name = result.advertisementData.advName.isNotEmpty
                ? result.advertisementData.advName : result.device.platformName;
            if (name == 'ESP32-PET') _found[result.device.remoteId.str] = result;
          }
        });
      });
      await FlutterBluePlus.startScan(
        withServices: [_serviceUuid],
        webOptionalServices: [_serviceUuid],
        timeout: const Duration(seconds: 12),
      );
      if (mounted && _device == null) {
        setState(() => _status = _found.isEmpty ? '找不到裝置' : '請選擇 ESP32-PET');
      }
    } catch (error) {
      if (mounted) setState(() => _status = '搜尋失敗');
      _message('搜尋失敗：$error');
    } finally {
      if (mounted) setState(() => _scanning = false);
    }
  }

  Future<void> _connect(BluetoothDevice device) async {
    if (_connecting || _device != null) return;
    setState(() { _connecting = true; _status = '連線中'; _deviceId = null; });
    try {
      await FlutterBluePlus.stopScan();
      await device.connect(license: License.nonprofit, timeout: const Duration(seconds: 15));
      final services = await device.discoverServices();
      final service = services.where((item) => item.uuid == _serviceUuid).firstOrNull;
      if (service == null) throw StateError('找不到 ESP32-PET BLE 服務');
      final command = service.characteristics.where((item) => item.uuid == _commandUuid).firstOrNull;
      final events = service.characteristics.where((item) => item.uuid == _eventUuid).firstOrNull;
      if (command == null || events == null) throw StateError('BLE 特徵值不完整');
      await _eventSubscription?.cancel();
      await events.setNotifyValue(true);
      _command = command;
      _device = device;
      _eventSubscription = events.onValueReceived.listen(_onFrame, onError: (Object e) {
        if (mounted) setState(() => _status = '資料通知中斷');
      });
      await _connectionSubscription?.cancel();
      _connectionSubscription = device.connectionState.listen((state) {
        if (state == BluetoothConnectionState.disconnected) _handleDisconnect(device);
      });
      if (mounted) setState(() { _status = '等待資料'; _waitingForData = true; });
      final info = await _request('get_device_info');
      if (info['ok'] != true || info['code'] != 'ok') {
        throw StateError('裝置拒絕查詢資訊：${info['code']}');
      }
      final statusResult = await _request('get_status');
      if (statusResult['ok'] != true || statusResult['code'] != 'ok') {
        throw StateError('裝置拒絕查詢狀態：${statusResult['code']}');
      }
      if (mounted && _pet == null) setState(() => _status = '等待資料');
    } catch (error) {
      await _cleanupConnection(device);
      if (mounted) setState(() { _device = null; _status = '尚未連線'; _waitingForData = false; });
      _message('連線失敗：$error');
    } finally {
      if (mounted) setState(() => _connecting = false);
    }
  }

  Future<void> _handleDisconnect(BluetoothDevice device) async {
    if (_device != device) return;
    await _cleanupConnection(device, disconnect: false);
    if (mounted) setState(() { _status = '已中斷連線'; _waitingForData = false; });
  }

  Future<void> _cleanupConnection(BluetoothDevice device, {bool disconnect = true}) async {
    await _eventSubscription?.cancel();
    _eventSubscription = null;
    await _connectionSubscription?.cancel();
    _connectionSubscription = null;
    _command = null;
    _device = null;
    _deviceId = null;
    _incoming.clear();
    _finishPending(StateError('BLE 連線中斷'));
    if (disconnect) {
      try { await device.disconnect(); } catch (_) {}
    }
  }

  Future<void> _disconnect() async {
    final device = _device;
    if (device != null) await _cleanupConnection(device);
    if (mounted) setState(() { _status = '尚未連線'; _waitingForData = false; });
  }

  Future<Map<String, dynamic>> _request(String command) async {
    final characteristic = _command;
    if (_device == null || characteristic == null) throw StateError('尚未連線');
    final id = ++_nextRequestId;
    final completer = Completer<Map<String, dynamic>>();
    _pending[id] = completer;
    try {
      final json = utf8.encode(jsonEncode({'v': 1, 'id': id, 'cmd': command}));
      if (json.length > _maxMessageBytes) throw StateError('命令超過協定長度');
      final count = (json.length + _fragmentBytes - 1) ~/ _fragmentBytes;
      final messageId = (++_nextMessageId) & 0xffff;
      for (var index = 0; index < count; index++) {
        final start = index * _fragmentBytes;
        final end = (start + _fragmentBytes < json.length) ? start + _fragmentBytes : json.length;
        final frame = <int>[messageId & 0xff, messageId >> 8, index, count, ...json.sublist(start, end)];
        await characteristic.write(frame, withoutResponse: false);
      }
      return await completer.future.timeout(const Duration(seconds: 8));
    } finally {
      _pending.remove(id);
    }
  }

  void _onFrame(List<int> bytes) {
    if (bytes.length < 5) return;
    final id = bytes[0] | (bytes[1] << 8);
    final index = bytes[2], count = bytes[3];
    final data = bytes.sublist(4);
    if (count == 0 || index >= count || data.isEmpty || data.length > _fragmentBytes) {
      _incoming.remove(id); return;
    }
    final now = DateTime.now();
    _incoming.removeWhere((_, message) => now.difference(message.started).inSeconds > 3);
    var message = _incoming[id];
    if (index == 0) {
      message = _IncomingMessage(count, now);
      _incoming[id] = message;
    }
    if (message == null || message.count != count || index != message.parts.length) {
      _incoming.remove(id); return;
    }
    message.parts.add(data);
    message.length += data.length;
    if (message.length > _maxMessageBytes) { _incoming.remove(id); return; }
    if (message.parts.length != count) return;
    _incoming.remove(id);
    final payload = message.parts.expand((part) => part).toList(growable: false);
    try {
      final decoded = jsonDecode(utf8.decode(payload));
      if (decoded is! Map<String, dynamic> || decoded['v'] != 1) return;
      _handleMessage(decoded);
    } on FormatException {
      if (mounted) _message('收到格式錯誤的 BLE 資料');
    }
  }

  void _handleMessage(Map<String, dynamic> data) {
    switch (data['type']) {
      case 'command_result':
        final id = data['id'];
        if (id is int) {
          final completer = _pending[id];
          if (completer != null && !completer.isCompleted) completer.complete(data);
        }
        break;
      case 'status':
        final deviceId = WebDeviceId.tryParse(data['device_id']);
        final snapshot = data['device_id'] != null && deviceId == null
            ? null
            : const BleSnapshotAdapter().decode(
                deviceId: deviceId,
                payload: data['pet'],
                receivedAt: DateTime.now(),
              );
        if (snapshot == null) {
          if (mounted) _message('ESP32 狀態欄位不完整或值無效');
          return;
        }
        if (mounted) {
          setState(() {
            _deviceId = snapshot.key.deviceId?.value;
            _pet = snapshot;
            _lastReceived = snapshot.receivedAt;
            _waitingForData = false;
            _status = '已連線';
          });
        }
        break;
      case 'device_info':
        if (data['protocol_version'] != 1 || data['name'] != 'ESP32-PET') {
          if (mounted) _message('裝置通訊版本不相容');
          return;
        }
        final deviceId = WebDeviceId.tryParse(data['device_id']);
        if (mounted) setState(() => _deviceId = deviceId?.value);
        if (data['device_id'] != null && deviceId == null) {
          _message('裝置 ID 格式無效');
        }
        break;
    }
  }

  void _finishPending(Object error) {
    for (final item in _pending.values) {
      if (!item.isCompleted) item.completeError(error);
    }
    _pending.clear();
  }

  void _message(String text) {
    if (!mounted) return;
    ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(text)));
  }

  String _age(Duration age) {
    final value = age.inSeconds;
    final days = value ~/ 86400;
    final hours = (value % 86400) ~/ 3600;
    final minutes = (value % 3600) ~/ 60;
    return days > 0 ? '$days 天 $hours 小時' : hours > 0 ? '$hours 小時 $minutes 分' : '$minutes 分';
  }

  String get _stateLabel {
    if (_pet == null) return _waitingForData ? '等待裝置狀態' : '尚無狀態資料';
    final pet = _pet!;
    if (pet.isDead) return '已死亡';
    if (pet.sleep == PetSleepState.sleeping) return '睡眠中';
    if (pet.health == PetHealthState.sick) return '生病';
    switch (pet.lifeStage) {
      case PetLifeStage.egg: return '蛋';
      case PetLifeStage.baby: return '幼鳥';
      case PetLifeStage.adult: return '成鳥';
    }
    return '未知狀態';
  }

  @override
  Widget build(BuildContext context) {
    final pet = _pet;
    final stale = pet != null && _device == null;
    return Scaffold(
      appBar: AppBar(title: const Text('ESP32 寵物'), actions: [
        IconButton(
          tooltip: _device == null ? '搜尋 BLE 裝置' : '中斷連線',
          onPressed: _device == null ? (_scanning || _connecting ? null : _scan) : _disconnect,
          icon: Icon(_device == null ? Icons.bluetooth_searching : Icons.bluetooth_connected),
        ),
        const SizedBox(width: 8),
      ]),
      body: Center(child: ConstrainedBox(
        constraints: const BoxConstraints(maxWidth: 620),
        child: ListView(padding: const EdgeInsets.all(18), children: [
          Card(child: Padding(padding: const EdgeInsets.all(16), child: Column(children: [
            Row(children: [
              const Icon(Icons.bluetooth), const SizedBox(width: 8),
              Expanded(child: Text(_status, style: const TextStyle(fontWeight: FontWeight.bold))),
              FilledButton.tonal(
                onPressed: _device == null ? (_scanning || _connecting ? null : _scan) : _disconnect,
                child: Text(_scanning ? '搜尋中…' : _connecting ? '連線中…' : _device == null ? '搜尋裝置' : '中斷'),
              ),
            ]),
            if (_device != null) ...[
              const SizedBox(height: 8),
              SelectableText(_deviceId == null
                  ? '裝置 ID 尚未提供'
                  : '裝置 ID：$_deviceId'),
            ],
            if (_found.isNotEmpty && _device == null) ...[
              const Divider(),
              for (final result in _found.values)
                ListTile(
                  dense: true, leading: const Icon(Icons.memory),
                  title: Text(result.device.platformName),
                  subtitle: Text(result.device.remoteId.str),
                  onTap: _connecting ? null : () => _connect(result.device),
                ),
            ],
          ]))),
          const SizedBox(height: 16),
          Card(color: const Color(0xffeab308), child: Padding(
            padding: const EdgeInsets.all(18), child: Column(children: [
              Text(stale ? '最後收到的資料 · 裝置離線' : _waitingForData && pet != null
                  ? '等待更新 · 顯示上次資料' : _device == null ? 'ESP32-PET · 唯讀' : 'ESP32-PET · 即時',
                  style: const TextStyle(fontWeight: FontWeight.w900)),
              const SizedBox(height: 12),
              Container(width: double.infinity, padding: const EdgeInsets.all(14),
                decoration: BoxDecoration(color: const Color(0xff9bbc0f), border: Border.all(color: const Color(0xff3b4b16), width: 3), borderRadius: BorderRadius.circular(14)),
                child: Column(children: [
                  Text(pet?.isDead == true ? '†' : pet?.lifeStage == PetLifeStage.egg ? '🥚' : pet?.lifeStage == PetLifeStage.baby ? '🐣' : pet?.lifeStage == PetLifeStage.adult ? '🐦' : '—', style: const TextStyle(fontSize: 48)),
                  Text(pet?.name ?? '等待 ESP32 狀態'),
                  Text(_stateLabel),
                  if (pet != null) Text('寵物 ID：${pet.key.petId}'),
                ]),
              ),
              if (_lastReceived != null) ...[
                const SizedBox(height: 8),
                Text('最後更新：${_lastReceived!.toLocal().toString().substring(0, 19)}', style: const TextStyle(fontSize: 12)),
              ],
            ]),
          )),
          const SizedBox(height: 16),
          Card(child: Padding(padding: const EdgeInsets.all(18), child: Column(
            crossAxisAlignment: CrossAxisAlignment.start, children: [
              const Text('寵物狀態', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
              const SizedBox(height: 12),
              _meter('飽食度', pet?.satietyPercent, Colors.amber),
              _meter('心情值', pet?.moodPercent, Colors.pink),
              _meter('清潔度', pet?.cleanlinessPercent, Colors.cyan),
              const Divider(),
              Text('年齡：${pet == null ? '--' : _age(pet.age)}'),
              Text('健康：${pet == null ? '--' : pet.health == PetHealthState.healthy ? '健康' : pet.health == PetHealthState.sick ? '生病' : '死亡'}'),
              Text('睡眠：${pet == null ? '--' : pet.sleep == PetSleepState.sleeping ? '睡眠中' : pet.sleep == PetSleepState.deepSleep ? '深度睡眠' : '清醒'}'),
            ],
          ))),
          const SizedBox(height: 16),
          const Text('階段 2：只讀狀態。遠端照顧與重置尚未啟用。', textAlign: TextAlign.center, style: TextStyle(color: Colors.black54)),
        ]),
      )),
    );
  }

  Widget _meter(String title, int? value, Color color) => Padding(
    padding: const EdgeInsets.symmetric(vertical: 7),
    child: Column(children: [
      Row(mainAxisAlignment: MainAxisAlignment.spaceBetween, children: [Text(title), Text(value == null ? '--' : '$value%')]),
      const SizedBox(height: 4),
      LinearProgressIndicator(value: value == null ? 0 : value / 100, color: color),
    ]),
  );
}

class _IncomingMessage {
  _IncomingMessage(this.count, this.started);
  final int count;
  final DateTime started;
  final List<List<int>> parts = [];
  int length = 0;
}

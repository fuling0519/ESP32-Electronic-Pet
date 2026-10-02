/* Capability guidance only; no permission prompt or claimed hardware connection. */
(()=>{'use strict';
window.checkBleReadiness=()=>{
  let summary,guidance;
  if(location.protocol==='file:'){
    summary='目前以本機檔案開啟：可試玩，尚不能作為正式 BLE 驗收環境。';
    guidance='BLE 整合時請改用 localhost 或 HTTPS 網站，並使用支援 Web Bluetooth 的瀏覽器。';
  }else if(!window.isSecureContext){
    summary='目前網站不是安全來源。';guidance='請改用 HTTPS，或在同一台電腦使用 localhost 開啟。';
  }else if(!navigator.bluetooth||typeof navigator.bluetooth.requestDevice!=='function'){
    summary='此瀏覽器目前沒有提供 Web Bluetooth。';guidance='請改用支援 Web Bluetooth 的瀏覽器與裝置組合；是否可連線仍須實機確認。展示模式可以繼續使用。';
  }else{
    summary='此瀏覽器提供 Web Bluetooth，開啟來源也符合基本要求。';guidance='這不代表系統藍牙已開啟或 ESP32 已連線；可按「搜尋 ESP32-PET」選擇裝置，讀取結果仍須實機驗收。';
  }
  document.querySelector('#ble-capability').textContent=summary;
  document.querySelector('#ble-guidance').textContent=guidance;
};
document.querySelector('#check-ble').onclick=window.checkBleReadiness;
})();


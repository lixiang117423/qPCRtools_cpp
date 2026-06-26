// app.js 的 jsdom 冒烟安全网。
// 用 index.html 的真实 DOM + mock（echarts/bootstrap）加载 i18n.js 与 app.js，
// 触发 DOMContentLoaded（qt 未定义 → standalone → initializeUI），断言：
//   1) 关键函数（function 声明）都挂到 window；
//   2) 脚本加载与 initializeUI 过程无未捕获异常。
// 作为 app.js 重构的安全网：重构后再跑，若仍 PASS 则行为大概率未变。
const fs = require('fs');
const path = require('path');
const { JSDOM, VirtualConsole } = require('jsdom');

const WEB_DIR = path.resolve(__dirname, '..', '..', 'web');
const indexHtml = fs.readFileSync(path.join(WEB_DIR, 'index.html'), 'utf8');
const i18nSrc = fs.readFileSync(path.join(WEB_DIR, 'js', 'i18n.js'), 'utf8');
const appSrc = fs.readFileSync(path.join(WEB_DIR, 'js', 'app.js'), 'utf8');
const templatesSrc = fs.readFileSync(path.join(WEB_DIR, 'js', 'templates.js'), 'utf8');

// app.js 在 setupFeatureCards 里 new bootstrap.Modal，故 mock 掉 echarts/bootstrap。
// （localStorage 用 http origin 由 jsdom 原生提供，无需 mock。）
const MOCKS = `
window.echarts = {
  init: function() { return { setOption(){}, resize(){}, clear(){}, dispose(){}, on(){} }; },
  registerTheme: function(){},
  connect: function(){}
};
const _ctor = function() { return { show(){}, hide(){}, update(){} }; };
window.bootstrap = { Modal: _ctor, Toast: _ctor, Dropdown: _ctor, Tooltip: _ctor };
`;

// 取 index.html，剥掉所有 <script>/<link>（不加载 jquery/bootstrap/echarts/qrc），
// 把 MOCKS + i18n + app 作为内联脚本注入 </body> 前，保证同步执行、无异步时序问题。
let html = indexHtml
  .replace(/<script[\s\S]*?<\/script>/g, '')
  .replace(/<link[^>]*>/g, '');
html = html.replace('</body>',
  `<script>${MOCKS}</script>` +
  `<script>${i18nSrc}</script>` +
  `<script>${appSrc}</script>` +
  `<script>${templatesSrc}</script>` +
  `</body>`);

const errors = [];
const vc = new VirtualConsole();
vc.on('jsdomError', e => errors.push(e));

const dom = new JSDOM(html, {
  runScripts: 'dangerously',
  url: 'http://localhost/',   // 非 opaque origin，jsdom 原生提供 localStorage
  virtualConsole: vc,
});

// function 声明会挂到 window；const（i18n/colorPalettes）不会，但若缺失会在 initializeUI 里抛 ReferenceError，被 errors 捕获。
const EXPECTED_FNS = [
  'initializeApplication', 'initializeUI',
  'loadExampleData', 'loadExampleCqData', 'loadExampleDesignData', 'loadExampleConcenData',
  'setupImportPage', 'setupAnalysisPage', 'setupResultsPage', 'setupStandardCurvePage',
  'navigateToPage', 'displayResults', 'displayCharts', 'parseCSV', 'convertToCSV', 'downloadFile',
  'exportResults', 'exportChart',
  'downloadCqTemplate', 'downloadDesignTemplate', 'downloadExampleDataFiles',
  'downloadDesignWithEffTemplate', 'downloadDesignWithSlopeTemplate', 'downloadStandardCurveExpExample',
];

// DOMContentLoaded 在 jsdom 里可能异步触发，稍候再断言。
setTimeout(() => {
  const w = dom.window;
  const missing = EXPECTED_FNS.filter(f => typeof w[f] !== 'function');
  const errorMessages = [...new Set(errors.map(e => (e.message || String(e)).split('\n')[0]))];

  let ok = true;
  if (missing.length) {
    ok = false;
    console.log('✗ Missing functions on window:', missing.join(', '));
  } else {
    console.log(`✓ All ${EXPECTED_FNS.length} expected functions present on window`);
  }
  if (errorMessages.length) {
    ok = false;
    console.log('✗ Uncaught errors during load / initializeUI:');
    errorMessages.forEach(m => console.log('   -', m));
  } else {
    console.log('✓ No uncaught errors (i18n + app.js loaded, initializeUI ran clean)');
  }

  console.log(ok ? '\nSMOKE PASS' : '\nSMOKE FAIL');
  process.exit(ok ? 0 : 1);
}, 300);

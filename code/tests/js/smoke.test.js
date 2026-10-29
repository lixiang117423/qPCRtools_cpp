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
const templatesSrc = fs.readFileSync(path.join(WEB_DIR, 'js', 'templates.js'), 'utf8');
const standardCurveSrc = fs.readFileSync(path.join(WEB_DIR, 'js', 'standard_curve.js'), 'utf8');
const appSrc = fs.readFileSync(path.join(WEB_DIR, 'js', 'app.js'), 'utf8');

// app.js 在 setupFeatureCards 里 new bootstrap.Modal，故 mock 掉 echarts/bootstrap。
// （localStorage 用 http origin 由 jsdom 原生提供，无需 mock。）
const MOCKS = `
window.__echartsCalls = { init: 0, options: [], lastElement: null };
window.echarts = {
  init: function(el) {
    window.__echartsCalls.init += 1;
    window.__echartsCalls.lastElement = el;
    return {
      setOption: function(o) { window.__echartsCalls.options.push(o); },
      resize: function(){}, clear: function(){}, dispose: function(){}, on: function(){},
      getDataURL: function() { return 'data:image/png;base64,AAAA'; }
    };
  },
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
  `<script>${templatesSrc}</script>` +
  `<script>${standardCurveSrc}</script>` +
  `<script>${appSrc}</script>` +
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
  'displayStandardCurveResults', 'runStandardCurveCalculation', 'exportStandardCurveResults',
  'loadScCqFile', 'loadScConcenFile', 'loadScCqExampleData', 'loadScConcenExampleData',
  'displayScCqPreview', 'displayScConcenPreview', 'generateMockStandardCurveResults',
  'navigateToPage', 'displayResults', 'displayCharts', 'parseCSV', 'convertToCSV', 'downloadFile',
  'exportResults', 'exportChart',
  'buildAnalysisRecord', 'exportAnalysisRecord', 'captureAnalysisParameters',
  'buildStandardCurveChartOption', 'displayStandardCurveChart', 'exportScChart',
  'buildQcReport', 'runQcChecks', 'displayQcReport',
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

  // 分析记录导出：buildAnalysisRecord() 必须是纯函数且只记录传入的真实值
  const recordSnapshot = {
    method: 'deltaDeltaCt', methodLabel: 'ΔΔCt Method',
    referenceGene: 'Beta Actin', controlGroup: '0',
    statisticalTest: 't.test', pAdjustMethod: 'holm', removeOutliers: false,
    cqFileName: 'cq.csv', cqRows: 96, cqColumns: 4,
    designFileName: 'design.csv', designRows: 96, designColumns: 5,
    summaryRows: 3, rawRows: 9, statisticCount: 2,
  };
  const record = w.buildAnalysisRecord(recordSnapshot, '1.1.0', '2026-10-15 15:20');
  const recordChecks = [
    ['header', record.includes('qPCRtools analysis record')],
    ['version', record.includes('1.1.0')],
    ['timestamp', record.includes('2026-10-15 15:20')],
    ['method', record.includes('ΔΔCt Method')],
    ['reference gene', record.includes('Beta Actin')],
    ['control group', record.includes('0')],
    ['statistical test', record.includes('t.test')],
    ['P-value adjustment', record.includes('holm')],
    ['input file names', record.includes('cq.csv') && record.includes('design.csv')],
    ['input shape', record.includes('96 x 4') && record.includes('96 x 5')],
    ['output counts', record.includes('Summary rows') && record.includes('Statistical results')],
    ['no undefined/NaN leakage', !/undefined|NaN/.test(record)],
  ];
  const recordFailures = recordChecks.filter(c => !c[1]).map(c => c[0]);
  if (recordFailures.length) {
    ok = false;
    console.log('✗ Analysis record content missing/invalid:', recordFailures.join(', '));
  } else {
    console.log(`✓ Analysis record contains all ${recordChecks.length} expected fields`);
  }

  if (w.buildAnalysisRecord(null, '1.1.0', 'x') !== '') {
    ok = false;
    console.log('✗ buildAnalysisRecord(null) must return an empty string');
  }

  if (!/adjusted within each gene/.test(record)) {
    ok = false;
    console.log('✗ Analysis record must describe within-gene P-value adjustment');
  }

  const scRecord = w.buildAnalysisRecord({
    method: 'standardCurve', methodLabel: 'Standard Curve',
    lowestConcen: 4, highestConcen: 4096, dilution: 4, byMean: true,
    cqFileName: 'calsc.cq.txt', cqRows: 24, cqColumns: 2,
    concenFileName: 'calsc.info.txt', concenRows: 24,
    summaryRows: 4, rawRows: 0, statisticCount: 0,
  }, '1.1.0', 't');
  if (!scRecord.includes('Dilution factor') || !scRecord.includes('Lowest concentration')
      || scRecord.includes('Reference gene(s)')) {
    ok = false;
    console.log('✗ Standard-curve record must list dilution/concentration parameters only');
  } else {
    console.log('✓ Standard-curve analysis record lists the standard-curve parameters only');
  }

  // 作图：buildExpressionChartOption() 必须用真实结果表建 option，displayCharts() 必须真的交给 echarts
  const chartResults = {
    table: [
      { Group: 'CK', Gene: 'G1', Mean: 1.0, StdDev: 0.10, Significance: '' },
      { Group: 'CK', Gene: 'G2', Mean: 1.0, StdDev: 0.20, Significance: '' },
      { Group: 'T',  Gene: 'G1', Mean: 2.5, StdDev: 0.30, Significance: '**' },
      { Group: 'T',  Gene: 'G2', Mean: 0.4, StdDev: 0.05, Significance: '*' },
    ],
    rawData: [
      { Gene: 'G1', Group: 'CK', Expression: 0.9 },
      { Gene: 'G1', Group: 'CK', Expression: 1.1 },
      { Gene: 'G2', Group: 'CK', Expression: 0.8 },
      { Gene: 'G2', Group: 'CK', Expression: 1.2 },
      { Gene: 'G1', Group: 'T', Expression: 2.4 },
      { Gene: 'G1', Group: 'T', Expression: 2.6 },
      { Gene: 'G2', Group: 'T', Expression: 0.3 },
      { Gene: 'G2', Group: 'T', Expression: 0.5 },
    ],
  };
  const chartOption = w.buildExpressionChartOption(chartResults, ['#111111', '#222222']);
  // 误差线必须真的画出来：ECharts 5.4.4 按"每项"调用 renderItem，且
  // renderItem 里的 params.data 是 undefined（必须用 api.value(dim)），
  // 每项应产出 3 条线（竖线 + 上下端帽）。2026-10-15 用 SSR 渲染实测确认：
  // 用 params.data 或 context.rendered 短路都会导致界面图里一根误差线都没有。
  const sdSeries = chartOption
    ? chartOption.series.find(s => s.type === 'custom' && s.name === 'SD')
    : null;
  const fakeApiFor = item => ({
    value: dim => (Array.isArray(item) ? item[dim] : item.value[dim]),
    size: () => [100, 0],
    coord: v => [v[0] * 100, 1000 - v[1] * 100],
  });
  const sdRendered = sdSeries
    ? sdSeries.data.map((d, i) => sdSeries.renderItem({ dataIndex: i }, fakeApiFor(d)))
    : [];
  const sdChildren = sdRendered
    .reduce((n, item) => n + (item && Array.isArray(item.children) ? item.children.length : 0), 0);
  // 误差线必须落在自己那根柱子的正中间：三条线共用一个 x，且相邻柱心间距 =
  // 柱宽 = band × (100 / 组数 × 0.7) / 100（band = api.size([1,0])[0]）。
  const band = 100;
  const nGroups = chartOption ? chartOption.series.filter(s => s.type === 'bar').length : 0;
  const barW = band * (100 / nGroups * 0.64) / 100;
  const barStep = barW * 1.12;
  const observationSeries = chartOption
    ? chartOption.series.filter(s => s.type === 'custom' && /observations$/.test(s.name))
    : [];
  const observationRendered = observationSeries.reduce((out, s) => {
    const groupIndex = ['CK', 'T'].indexOf(s.name.replace(/ observations$/, ''));
    s.data.forEach((d, i) => {
      const item = s.renderItem({ dataIndex: i }, fakeApiFor(d));
      const expectedX = d[0] * band
        + (groupIndex - (nGroups - 1) / 2) * barStep
        + d[2] * barW;
      out.push({ groupIndex: groupIndex, item: item, expectedX: expectedX });
    });
    return out;
  }, []);
  const observationGeometryOk = observationSeries.length === 2
    && observationRendered.length === 8
    && observationRendered.every(entry =>
      Number.isFinite(entry.item.shape.cx)
      && Math.abs(entry.item.shape.cx - entry.expectedX) < 1e-9
      && entry.groupIndex >= 0
    );
  const labelSeries = chartOption
    ? chartOption.series.filter(s => s.type === 'custom' && /significance$/.test(s.name))
    : [];
  const labelsRendered = labelSeries.reduce((out, s) => {
    const groupIndex = ['CK', 'T'].indexOf(s.name.replace(/ significance$/, ''));
    s.data.forEach((d, i) => {
      const item = s.renderItem({ dataIndex: i }, fakeApiFor(d));
      const values = Array.isArray(d) ? d : d.value;
      const expectedX = values[0] * band
        + (groupIndex - (nGroups - 1) / 2) * barStep;
      out.push({ item: item, expectedX: expectedX, text: values[2] });
    });
    return out;
  }, []);
  const labelsCentered = labelSeries.length === 1 && labelsRendered.length === 2
    && labelsRendered.every(entry =>
      Number.isFinite(entry.item.style.x)
      && Math.abs(entry.item.style.x - entry.expectedX) < 1e-9
      && entry.item.style.text === entry.text
    );
  const sdCentered = !!sdSeries && sdSeries.data.length > 0 && sdSeries.data.every((d, i) => {
    const item = sdRendered[i];
    if (!item) return false;
    const expectedX = (d[0] * 100) + (d[2] - (nGroups - 1) / 2) * barStep;
    const xs = item.children.map(c => c.shape.x1);
    return Math.abs(xs[0] - expectedX) < 1e-9
      && xs[1] === xs[2]
      && xs[0] > xs[1];
  });
  const chartChecks = [
    ['option built', !!chartOption],
    ['genes as categories',
      chartOption && JSON.stringify(chartOption.xAxis.data) === JSON.stringify(['G1', 'G2'])],
    ['one bar series per group',
      chartOption && chartOption.series.filter(s => s.type === 'bar').length === 2],
    ['legend excludes error bars and observations',
      chartOption && JSON.stringify(chartOption.legend.data) === JSON.stringify(['CK', 'T'])],
    ['bars have a visible gap',
      chartOption && chartOption.series.filter(s => s.type === 'bar').every(s => s.barGap === '12%')],
    ['control means from table',
      chartOption && JSON.stringify(chartOption.series[0].data) === JSON.stringify([1, 1])],
    ['treatment means from table',
      chartOption && JSON.stringify(
        chartOption.series.find(s => s.type === 'bar' && s.name === 'T').data
      ) === JSON.stringify([2.5, 0.4])],
    ['SD error bars (custom series, one per cell)',
      !!sdSeries && sdSeries.data.length === 4],
    ['SD error bars actually rendered (3 lines per cell)',
      sdChildren === 12],
    ['SD error bars centred on their own bar',
      sdCentered],
    ['individual biological replicates',
      observationSeries.reduce((n, s) => n + s.data.length, 0) === 8],
    ['observations use custom bar-relative geometry',
      observationGeometryOk],
    ['significance labels use custom bar-relative geometry',
      labelsCentered],
    ['palette applied',
      chartOption && chartOption.series[0].itemStyle.color === '#111111'],
  ];
  const chartFailures = chartChecks.filter(c => !c[1]).map(c => c[0]);
  if (chartFailures.length) {
    ok = false;
    console.log('✗ Chart option invalid:', chartFailures.join(', '));
  } else {
    console.log(`✓ Chart option contains all ${chartChecks.length} expected elements`);
  }

  const optionsBefore = w.__echartsCalls.options.length;
  const rendered = w.displayCharts(chartResults);
  if (rendered !== true || w.__echartsCalls.options.length !== optionsBefore + 1) {
    ok = false;
    console.log('✗ displayCharts() must hand one option to echarts.init().setOption()');
  } else {
    console.log('✓ displayCharts() passes the built option to ECharts');
  }

  // 逐重复表（无 Mean 列）也要能作图：先按 gene×group 汇总
  const perRepResults = {
    table: [
      { Group: 'CK', Gene: 'G1', Expression: 1.0 },
      { Group: 'CK', Gene: 'G1', Expression: 1.2 },
      { Group: 'T', Gene: 'G1', Expression: 3.0 },
      { Group: 'T', Gene: 'G1', Expression: 2.0 },
    ],
  };
  const perRepOption = w.buildExpressionChartOption(perRepResults, ['#333333']);
  const perRepMean = perRepOption && perRepOption.series[0].data[0];
  if (!(Math.abs(perRepMean - 1.1) < 1e-9)) {
    ok = false;
    console.log('✗ Per-replicate table must be aggregated before plotting (mean =', perRepMean, ')');
  } else {
    console.log('✓ Per-replicate tables are aggregated (mean and SD) before plotting');
  }

  // 标准曲线图：log_d(Conc) 横轴 + 由 slope/intercept 画的回归线 + 范围外空心点
  const scChartResults = { table: [{ Gene: 'G1', Slope: -2.0, Intercept: 30, R2: 0.99 }] };
  const scCq = [{ Position: 'A1', Cq: 26 }, { Position: 'A2', Cq: 28 }, { Position: 'A3', Cq: 38 }];
  const scConcen = [
    { Position: 'A1', Gene: 'G1', Conc: 256 },
    { Position: 'A2', Gene: 'G1', Conc: 64 },
    { Position: 'A3', Gene: 'G1', Conc: 1 },
  ];
  const scOption = w.buildStandardCurveChartOption(scChartResults, scCq, scConcen, 4, 4, 4096, ['#444444']);
  const scLine = scOption && scOption.series.find(s => s.type === 'line');
  const scScatters = scOption ? scOption.series.filter(s => s.type === 'scatter') : [];
  const scLineData = scLine ? scLine.data : [];
  const scChecks = [
    ['option built', !!scOption],
    ['x axis on log4 scale', scOption && scOption.xAxis.name === 'log4(concentration)'],
    ['y axis is Cq', scOption && scOption.yAxis.name === 'Cq'],
    ['in-range scatter has 2 points', scScatters[0] && scScatters[0].data.length === 2],
    ['out-of-range scatter has 1 point', scScatters[1] && scScatters[1].data.length === 1],
    ['regression line from slope/intercept',
      scLineData.length === 2 && Math.abs(scLineData[0][0] - 0) < 1e-9
      && Math.abs(scLineData[0][1] - 30) < 1e-9 && Math.abs(scLineData[1][1] - 22) < 1e-9],
  ];
  const scFailures = scChecks.filter(c => !c[1]).map(c => c[0]);
  if (scFailures.length) {
    ok = false;
    console.log('✗ Standard-curve chart option invalid:', scFailures.join(', '));
  } else {
    console.log(`✓ Standard-curve chart option contains all ${scChecks.length} expected elements`);
  }

  // displayStandardCurveChart() 从页面状态（scResults/scCqData/scConcenData）取数并交给 ECharts
  try {
    const scOptionsBefore = w.__echartsCalls.options.length;
    w.eval('scResults = ' + JSON.stringify(scChartResults) + ';'
      + 'scCqData = ' + JSON.stringify(scCq) + ';'
      + 'scConcenData = ' + JSON.stringify(scConcen) + ';');
    const scRendered = w.displayStandardCurveChart();
    if (scRendered !== true || w.__echartsCalls.options.length !== scOptionsBefore + 1) {
      ok = false;
      console.log('✗ displayStandardCurveChart() must hand one option to ECharts');
    } else {
      console.log('✓ displayStandardCurveChart() passes the built option to ECharts');
    }
  } catch (e) {
    ok = false;
    console.log('✗ displayStandardCurveChart() threw:', e.message);
  }

  // 质控预警：三类检查必须各按阈值触发，且"干净数据"不得误报
  const qcInput = {
    cqData: [
      { Position: 'A1', Gene: 'G1', Cq: 20.0 },
      { Position: 'A2', Gene: 'G1', Cq: 20.1 },
      { Position: 'B1', Gene: 'G2', Cq: 40.0 },   // > 35 → 触发 Cq 上限；同时与 B2 同属一个样本
      { Position: 'B2', Gene: 'G2', Cq: 20.0 },   // 技术重复 SD ≈ 14 → 触发离散度预警
    ],
    designData: [
      { Position: 'A1', Group: 'CK', BioRep: '1' },
      { Position: 'A2', Group: 'CK', BioRep: '1' },
      { Position: 'B1', Group: 'T', BioRep: '1' },
      { Position: 'B2', Group: 'T', BioRep: '1' },
    ],
    scResults: { table: [{ Gene: 'G3', Slope: -2.0, Intercept: 30, Efficiency: 0.80 }] },
  };
  const qcReport = w.buildQcReport(qcInput);
  const codes = qcReport.items.map(i => i.code).sort().join(',');
  const qcChecks = [
    ['three warnings', qcReport.items.length === 3],
    ['codes', codes === 'cqHigh,efficiency,techRepSd'],
    ['efficiency names the primer', /G3/.test(qcReport.items.find(i => i.code === 'efficiency').message)],
    ['thresholds echoed', qcReport.thresholds.cqUpper === 35 && qcReport.thresholds.techRepSd === 0.5],
  ];
  const qcFailures = qcChecks.filter(c => !c[1]).map(c => c[0]);
  if (qcFailures.length) {
    ok = false;
    console.log('✗ QC report invalid:', qcFailures.join(', '));
  } else {
    console.log(`✓ QC report raises the ${qcReport.items.length} expected warnings at the default thresholds`);
  }

  const cleanReport = w.buildQcReport({
    cqData: [
      { Position: 'A1', Gene: 'G1', Cq: 20.0 },
      { Position: 'A2', Gene: 'G1', Cq: 20.1 },
    ],
    designData: [
      { Position: 'A1', Group: 'CK', BioRep: '1' },
      { Position: 'A2', Group: 'CK', BioRep: '1' },
    ],
    scResults: { table: [{ Gene: 'G3', Efficiency: 0.95 }] },
  });
  if (cleanReport.items.length !== 0) {
    ok = false;
    console.log('✗ Clean data must not raise QC warnings (got', cleanReport.items.length, ')');
  } else {
    console.log('✓ Clean data raise no QC warnings (no false positives)');
  }

  // 分析记录必须带上质控段
  const recordWithQc = w.buildAnalysisRecord(
    Object.assign({}, recordSnapshot, { qcItems: qcReport.items }), '1.2.0', 't');
  if (!recordWithQc.includes('[Quality control warnings]')
      || !recordWithQc.includes('well(s) have Cq > 35')) {
    ok = false;
    console.log('✗ Analysis record must include the QC warnings');
  } else {
    console.log('✓ Analysis record carries the QC warnings');
  }

  // P 值校正控件只在有组间比较时显示；标准曲线参数计算不显示。
  const standardCurveRadio = w.document.querySelector('input[name="analysisMethod"][value="standardCurve"]');
  standardCurveRadio.checked = true;
  w.updateAnalysisMethodUI();
  const pAdjustHiddenOnCurve =
    w.document.getElementById('pAdjustField').style.display === 'none';
  const deltaDeltaRadio = w.document.querySelector('input[name="analysisMethod"][value="deltaDeltaCt"]');
  deltaDeltaRadio.checked = true;
  w.updateAnalysisMethodUI();
  const pAdjustShownOnDdct =
    w.document.getElementById('pAdjustField').style.display === 'block';
  if (!pAdjustHiddenOnCurve || !pAdjustShownOnDdct) {
    ok = false;
    console.log('✗ P-value adjustment control has incorrect visibility by method');
  } else {
    console.log('✓ P-value adjustment control visibility follows the analysis method');
  }

  console.log(ok ? '\nSMOKE PASS' : '\nSMOKE FAIL');
  process.exit(ok ? 0 : 1);
}, 300);

// 标准曲线页面的加载、计算、展示与导出函数（从 app.js 抽取，行为不变）。
// 作为独立 <script> 加载，与 app.js 共享全局（bridge / scCqData / scConcenData /
// scResults / showNotification / i18n / parseCSV / arrayBufferToBase64 /
// renderTablePreview / downloadFile 等，均在调用时解析）。

/**
 * Setup Standard Curve page
 */
function setupStandardCurvePage() {
    console.log('Setting up Standard Curve page...');

    // Cq Data loading
    document.getElementById('loadScCqBtn').addEventListener('click', async function() {
        const fileInput = document.getElementById('scCqFileInput');
        if (fileInput.files.length === 0) {
            showNotification(i18n.t('msg.noFileSelected'), 'warning');
            return;
        }

        const file = fileInput.files[0];
        const fileNameLower = file.name.toLowerCase();
        const isCsv = fileNameLower.endsWith('.csv');
        const isExcel = fileNameLower.endsWith('.xlsx') || fileNameLower.endsWith('.xls');
        if (!isCsv && !isExcel) {
            showNotification('Invalid file format', 'warning');
            return;
        }

        if (bridge) {
            const reader = new FileReader();
            reader.onload = async function(e) {
                try {
                    if (isCsv) {
                        const content = e.target.result;
                        const result = await bridge.loadCqFromContent(content);
                        const parsed = JSON.parse(result);
                        if (parsed && parsed.error) {
                            showNotification(parsed.error, 'danger');
                            return;
                        }
                        scCqData = parsed.data;
                        scCqData.columns = parsed.columns;
                    } else {
                        const buffer = e.target.result;
                        const base64 = arrayBufferToBase64(buffer);
                        const result = await bridge.loadCqExcelFromBase64(base64, 0, true);
                        const parsed = JSON.parse(result);
                        if (parsed && parsed.error) {
                            showNotification(parsed.error, 'danger');
                            return;
                        }
                        scCqData = parsed.data;
                        scCqData.columns = parsed.columns;
                    }
                    displayScCqPreview(scCqData);
                    showNotification(i18n.t('msg.dataLoaded'), 'success');
                } catch (error) {
                    console.error('Error loading Cq file:', error);
                    showNotification('Failed to parse file: ' + error.message, 'danger');
                }
            };
            if (isCsv) {
                reader.readAsText(file);
            } else {
                reader.readAsArrayBuffer(file);
            }
        } else {
            if (isExcel) {
                showNotification('Excel import is only supported inside the desktop app.', 'warning');
                return;
            }
            loadScCqFile(file);
        }
    });

    // Load Cq example data
    document.getElementById('loadScCqExampleBtn').addEventListener('click', function() {
        loadScCqExampleData();
    });

    // Concentration Data loading
    document.getElementById('loadScConcenBtn').addEventListener('click', async function() {
        const fileInput = document.getElementById('scConcenFileInput');
        if (fileInput.files.length === 0) {
            showNotification(i18n.t('msg.noFileSelected'), 'warning');
            return;
        }

        const file = fileInput.files[0];
        const fileNameLower = file.name.toLowerCase();
        const isCsv = fileNameLower.endsWith('.csv');
        const isExcel = fileNameLower.endsWith('.xlsx') || fileNameLower.endsWith('.xls');
        if (!isCsv && !isExcel) {
            showNotification('Invalid file format', 'warning');
            return;
        }

        if (bridge) {
            const reader = new FileReader();
            reader.onload = async function(e) {
                try {
                    if (isCsv) {
                        const content = e.target.result;
                        const result = await bridge.loadConcenFromContent(content);
                        const parsed = JSON.parse(result);
                        if (parsed && parsed.error) {
                            showNotification(parsed.error, 'danger');
                            return;
                        }
                        scConcenData = parsed.data;
                        scConcenData.columns = parsed.columns;
                    } else {
                        const buffer = e.target.result;
                        const base64 = arrayBufferToBase64(buffer);
                        const result = await bridge.loadConcenExcelFromBase64(base64, 0, true);
                        const parsed = JSON.parse(result);
                        if (parsed && parsed.error) {
                            showNotification(parsed.error, 'danger');
                            return;
                        }
                        scConcenData = parsed.data;
                        scConcenData.columns = parsed.columns;
                    }
                    displayScConcenPreview(scConcenData);
                    showNotification(i18n.t('msg.dataLoaded'), 'success');
                } catch (error) {
                    console.error('Error loading Concentration file:', error);
                    showNotification('Failed to parse file: ' + error.message, 'danger');
                }
            };
            if (isCsv) {
                reader.readAsText(file);
            } else {
                reader.readAsArrayBuffer(file);
            }
        } else {
            if (isExcel) {
                showNotification('Excel import is only supported inside the desktop app.', 'warning');
                return;
            }
            loadScConcenFile(file);
        }
    });

    // Load Concentration example data
    document.getElementById('loadScConcenExampleBtn').addEventListener('click', function() {
        loadScConcenExampleData();
    });

    // Run Standard Curve calculation
    document.getElementById('runStandardCurve').addEventListener('click', async function() {
        await runStandardCurveCalculation();
    });

    // Export buttons
    document.getElementById('exportScCsv').addEventListener('click', function() {
        exportStandardCurveResults('csv');
    });

    document.getElementById('exportScExcel').addEventListener('click', function() {
        exportStandardCurveResults('excel');
    });

    console.log('Standard Curve page setup complete');
}

/**
 * Load Standard Curve Cq file
 */
function loadScCqFile(file) {
    const reader = new FileReader();
    reader.onload = function(e) {
        try {
            const data = parseCSV(e.target.result);
            scCqData = data;
            displayScCqPreview(data);
            showNotification(i18n.t('msg.dataLoaded'), 'success');
        } catch (error) {
            showNotification(i18n.t('msg.error') + ': ' + error.message, 'danger');
        }
    };
    reader.readAsText(file);
}

/**
 * Load Standard Curve Concentration file
 */
function loadScConcenFile(file) {
    const reader = new FileReader();
    reader.onload = function(e) {
        try {
            const data = parseCSV(e.target.result);
            scConcenData = data;
            displayScConcenPreview(data);
            showNotification(i18n.t('msg.dataLoaded'), 'success');
        } catch (error) {
            showNotification(i18n.t('msg.error') + ': ' + error.message, 'danger');
        }
    };
    reader.readAsText(file);
}

/**
 * Load Standard Curve Cq example data
 */
async function loadScCqExampleData() {
    try {
        const response = await fetch('calsc.cq.txt');
        if (!response.ok) {
            throw new Error('Failed to load example Cq data file');
        }
        const csvText = await response.text();
        const data = parseCSV(csvText);

        // Cq data file has Position and Cq, need to add Gene column
        // We'll add Gene info based on Position pattern
        scCqData = data.map(row => {
            const position = row.Position;
            let gene = '';
            // Determine gene based on position number (1-3 for Gene1, 4-6 for Gene2, etc.)
            const posNum = parseInt(position.match(/\d+/)[0], 10);  // 字符串需转数值比较
            if (posNum >= 1 && posNum <= 3) gene = 'Gene1';
            else if (posNum >= 4 && posNum <= 6) gene = 'Gene2';
            else if (posNum >= 7 && posNum <= 9) gene = 'Gene3';
            else if (posNum >= 10 && posNum <= 12) gene = 'Gene4';

            return {
                Position: position,
                Gene: gene,
                Cq: parseFloat(row.Cq)
            };
        });

        displayScCqPreview(scCqData);
        showNotification('Example Cq data loaded (calsc.cq.txt)', 'success');
    } catch (error) {
        console.error('Error loading example Cq data:', error);
        // Fallback to embedded data
        scCqData = [
            { Position: 'A1', Gene: 'Gene1', Cq: 27.26 },
            { Position: 'A2', Gene: 'Gene1', Cq: 27.10 },
            { Position: 'A3', Gene: 'Gene1', Cq: 27.47 },
            { Position: 'B1', Gene: 'Gene1', Cq: 27.41 },
            { Position: 'B2', Gene: 'Gene1', Cq: 27.33 },
            { Position: 'B3', Gene: 'Gene1', Cq: 27.42 },
            { Position: 'A4', Gene: 'Gene2', Cq: 21.20 },
            { Position: 'A5', Gene: 'Gene2', Cq: 21.11 },
            { Position: 'A6', Gene: 'Gene2', Cq: 21.06 },
            { Position: 'B4', Gene: 'Gene2', Cq: 22.36 },
            { Position: 'B5', Gene: 'Gene2', Cq: 22.31 },
            { Position: 'B6', Gene: 'Gene2', Cq: 22.35 }
        ];
        displayScCqPreview(scCqData);
        showNotification('Example Cq data loaded (embedded)', 'success');
    }
}

/**
 * Load Standard Curve Concentration example data
 */
async function loadScConcenExampleData() {
    try {
        const response = await fetch('calsc.info.txt');
        if (!response.ok) {
            throw new Error('Failed to load example concentration data file');
        }
        const csvText = await response.text();
        scConcenData = parseCSV(csvText);

        displayScConcenPreview(scConcenData);
        showNotification('Example Concentration data loaded (calsc.info.txt)', 'success');
    } catch (error) {
        console.error('Error loading example concentration data:', error);
        // Fallback to embedded data
        scConcenData = [
            { Position: 'A1', Gene: 'Gene1', Conc: 16384 },
            { Position: 'A2', Gene: 'Gene1', Conc: 16384 },
            { Position: 'A3', Gene: 'Gene1', Conc: 16384 },
            { Position: 'B1', Gene: 'Gene1', Conc: 4096 },
            { Position: 'B2', Gene: 'Gene1', Conc: 4096 },
            { Position: 'B3', Gene: 'Gene1', Conc: 4096 },
            { Position: 'C1', Gene: 'Gene1', Conc: 1024 },
            { Position: 'C2', Gene: 'Gene1', Conc: 1024 },
            { Position: 'C3', Gene: 'Gene1', Conc: 1024 },
            { Position: 'D1', Gene: 'Gene1', Conc: 256 },
            { Position: 'D2', Gene: 'Gene1', Conc: 256 },
            { Position: 'D3', Gene: 'Gene1', Conc: 256 }
        ];
        displayScConcenPreview(scConcenData);
        showNotification('Example Concentration data loaded (embedded)', 'success');
    }
}

// SC preview wrappers using the shared renderTablePreview
function displayScCqPreview(data)     { renderTablePreview('scCqPreviewTable',      data, 'SC Cq'); }
function displayScConcenPreview(data) { renderTablePreview('scConcenPreviewTable',  data, 'SC Concen'); }

/**
 * Run Standard Curve calculation
 */
async function runStandardCurveCalculation() {
    // Validate data
    if (!scCqData || scCqData.length === 0) {
        showNotification('Please load Cq data first', 'warning');
        return;
    }

    if (!scConcenData || scConcenData.length === 0) {
        showNotification('Please load Concentration data first', 'warning');
        return;
    }

    // Get parameters
    const lowestConcen = parseFloat(document.getElementById('scLowestConcen').value);
    const highestConcen = parseFloat(document.getElementById('scHighestConcen').value);
    const dilution = parseFloat(document.getElementById('scDilution').value);
    const byMean = document.getElementById('scByMean').checked;

    // Validate parameters
    if (isNaN(lowestConcen) || isNaN(highestConcen) || isNaN(dilution)) {
        showNotification('Invalid parameters', 'warning');
        return;
    }

    if (lowestConcen >= highestConcen) {
        showNotification('Lowest concentration must be less than highest concentration', 'warning');
        return;
    }

    const params = {
        lowestConcen: lowestConcen,
        highestConcen: highestConcen,
        dilution: dilution,
        byMean: byMean
    };

    if (bridge) {
        try {
            // Send data to C++
            const cqJson = JSON.stringify(scCqData);
            const concenJson = JSON.stringify(scConcenData);

            await bridge.setCqData(cqJson);
            await bridge.setConcenData(concenJson);

            // Run calculation
            const result = await bridge.calculateStandardCurve(JSON.stringify(params));
            scResults = JSON.parse(result);

            displayStandardCurveResults(scResults);
            showNotification('Standard curve calculation completed', 'success');
        } catch (error) {
            showNotification('Calculation failed: ' + error.message, 'danger');
        }
    } else {
        // Demo mode
        scResults = generateMockStandardCurveResults();
        displayStandardCurveResults(scResults);
        showNotification('Standard curve calculation completed (demo mode)', 'success');
    }
}

/**
 * Display Standard Curve results
 */
function displayStandardCurveResults(results) {
    const section = document.getElementById('scResultsSection');
    const table = document.getElementById('scResultsTable');
    const thead = table.querySelector('thead');
    const tbody = table.querySelector('tbody');

    if (!results || !results.table || results.table.length === 0) {
        showNotification('No results to display', 'warning');
        return;
    }

    section.style.display = 'block';

    const headers = ['Gene', 'Formula', 'Slope', 'Intercept', 'R2', 'PValue', 'Efficiency', 'MinCq', 'MaxCq'];

    thead.innerHTML = '';
    const headerRow = document.createElement('tr');
    headers.forEach(header => {
        const th = document.createElement('th');
        th.textContent = header;
        headerRow.appendChild(th);
    });
    thead.appendChild(headerRow);

    tbody.innerHTML = '';
    results.table.forEach(row => {
        const tr = document.createElement('tr');
        headers.forEach(header => {
            const td = document.createElement('td');
            const value = row[header];
            if (typeof value === 'number') {
                if (header === 'Slope' || header === 'Intercept') {
                    td.textContent = value.toFixed(2);
                } else if (header === 'R2') {
                    td.textContent = value.toFixed(4);
                } else if (header === 'PValue') {
                    td.textContent = value.toExponential(5);
                } else if (header === 'Efficiency') {
                    td.textContent = value.toFixed(3);
                } else {
                    td.textContent = value.toFixed(2);
                }
            } else {
                td.textContent = value || '';
            }
            tr.appendChild(td);
        });
        tbody.appendChild(tr);
    });
}

/**
 * Export Standard Curve results
 */
function exportStandardCurveResults(format) {
    if (!scResults || !scResults.table || scResults.table.length === 0) {
        showNotification(i18n.t('msg.noData'), 'warning');
        return;
    }

    const headers = ['Gene', 'Formula', 'Slope', 'Intercept', 'R2', 'PValue', 'Efficiency', 'MinCq', 'MaxCq'];

    if (bridge) {
        // 桌面端走 C++ 导出（Excel 输出真正的 xlsx 语义：带 BOM 的 CSV 内容；
        // 旧实现把 CSV 内容下载成 .xlsx 文件，Excel 无法打开）
        const payload = { table: scResults.table };
        bridge.showSaveDialog(i18n.t('btn.export'), format === 'csv' ? '*.csv' : '*.xlsx', 'standard_curve_results').then(filePath => {
            if (filePath) {
                if (format === 'csv') {
                    bridge.exportToCSV(JSON.stringify(payload), filePath);
                } else {
                    bridge.exportToExcel(JSON.stringify(payload), filePath);
                }
                showNotification(i18n.t('msg.analysisCompleted'), 'success');
            }
        });
    } else {
        // 浏览器 demo 模式：CSV 下载（不再伪造 .xlsx 扩展名）
        let content = headers.join(',') + '\n';
        scResults.table.forEach(row => {
            const values = headers.map(h => (row[h] === null || row[h] === undefined) ? '' : row[h]);
            content += values.join(',') + '\n';
        });
        downloadFile(content, 'standard_curve_results.csv', 'text/csv');
        showNotification(i18n.t('msg.analysisCompleted'), 'success');
    }
}

/**
 * Generate mock Standard Curve results for demo mode
 */
function generateMockStandardCurveResults() {
    return {
        method: 'standardCurve',
        table: [
            {
                Gene: 'Gene1',
                Formula: 'y = -3.32x + 35.21',
                Slope: -3.32,
                Intercept: 35.21,
                R2: 0.9987,
                PValue: 1.23e-08,
                Efficiency: 1.998,
                MinCq: 9.3,
                MaxCq: 25.3
            },
            {
                Gene: 'Gene2',
                Formula: 'y = -3.28x + 34.87',
                Slope: -3.28,
                Intercept: 34.87,
                R2: 0.9975,
                PValue: 3.45e-08,
                Efficiency: 2.015,
                MinCq: 10.2,
                MaxCq: 26.1
            }
        ]
    };
}

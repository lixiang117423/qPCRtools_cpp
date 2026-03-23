/**
 * qPCRtools Main Application
 * Handles UI interactions and C++ bridge communication
 */

// Global variables
let bridge = null;
let currentCqData = null;
let currentDesignData = null;
let analysisResults = null;
let mainChart = null;

// Color palettes for ggplot2 style
const colorPalettes = {
    nature: ['#E64B35', '#4DBBD5', '#00A087', '#3C5488', '#F39B7F', '#8491B4', '#91D1C2', '#B09C85'],
    science: ['#3C5488', '#F39B7F', '#8491B4', '#91D1C2', '#DC0000', '#7E6148', '#B09C85', '#00A087'],
    lancet: ['#4C9553', '#008080', '#7FB800', '#9A4800', '#F6511D'],
    jco: ['#0073C2', '#E69F00', '#ABABAB', '#878787', '#D55E00', '#CC79A7'],
    nejm: ['#3C5488', '#F39B7F', '#00A087', '#E64B35', '#91D1C2'],
    jama: ['#3C5488', '#4DBBD5', '#00A087', '#F39B7F', '#E64B35', '#8491B4']
};

/**
 * Initialize application when WebChannel is ready
 */
function initializeApplication() {
    console.log('Initializing qPCRtools...');

    // Get bridge object from Qt
    if (typeof qt !== 'undefined') {
        new QWebChannel(qt.webChannelTransport, function(channel) {
            bridge = channel.objects.bridge;

            // Connect signals from C++
            bridge.dataLoaded.connect(onDataLoaded);
            bridge.calculationCompleted.connect(onCalculationCompleted);
            bridge.errorOccurred.connect(onErrorOccurred);
            bridge.progressChanged.connect(onProgressChanged);

            console.log('Bridge connected successfully');

            // Initialize UI
            initializeUI();
        });
    } else {
        console.warn('Running in web browser mode (no Qt bridge)');
        // Still initialize UI for testing
        initializeUI();
    }
}

/**
 * Initialize UI components and event handlers
 */
function initializeUI() {
    // Apply translations
    i18n.updateUIText();
    i18n.updateLanguageDisplay();

    // Navigation
    setupNavigation();

    // Page-specific initialization
    setupImportPage();
    setupAnalysisPage();
    setupResultsPage();

    // Feature cards click handlers
    setupFeatureCards();

    // Homepage buttons
    setupHomepageButtons();

    // Language switcher
    setupLanguageSwitcher();

    console.log('Application initialized');
}

/**
 * Setup feature cards click handlers
 */
function setupFeatureCards() {
    console.log('Setting up feature cards...');
    document.querySelectorAll('.feature-card').forEach(card => {
        card.style.cursor = 'pointer';
        card.addEventListener('click', function() {
            console.log('Feature card clicked!');
            const action = this.getAttribute('data-action');
            if (action === 'start-analysis') {
                navigateToPage('import');
            }
        });
    });
}

/**
 * Setup homepage buttons
 */
function setupHomepageButtons() {
    console.log('Setting up homepage buttons...');

    // Download Cq template (from homepage)
    const downloadCqBtn = document.getElementById('downloadCqTemplate');
    if (downloadCqBtn) {
        downloadCqBtn.addEventListener('click', function(e) {
            e.preventDefault();
            downloadCqTemplate();
        });
    }

    // Download Design template (from homepage)
    const downloadDesignBtn = document.getElementById('downloadDesignTemplate');
    if (downloadDesignBtn) {
        downloadDesignBtn.addEventListener('click', function(e) {
            e.preventDefault();
            downloadDesignTemplate();
        });
    }

    // Download example data (from homepage)
    const downloadExampleBtn = document.getElementById('downloadExampleData');
    if (downloadExampleBtn) {
        downloadExampleBtn.addEventListener('click', function(e) {
            e.preventDefault();
            downloadExampleDataFiles();
        });
    }

    // Modal download links
    const modalDownloadCqBtn = document.getElementById('modalDownloadCqTemplate');
    if (modalDownloadCqBtn) {
        modalDownloadCqBtn.addEventListener('click', function(e) {
            e.preventDefault();
            downloadCqTemplate();
        });
    }

    const modalDownloadDesignBtn = document.getElementById('modalDownloadDesignTemplate');
    if (modalDownloadDesignBtn) {
        modalDownloadDesignBtn.addEventListener('click', function(e) {
            e.preventDefault();
            downloadDesignTemplate();
        });
    }

    const modalDownloadExampleBtn = document.getElementById('modalDownloadExampleData');
    if (modalDownloadExampleBtn) {
        modalDownloadExampleBtn.addEventListener('click', function(e) {
            e.preventDefault();
            downloadExampleDataFiles();
        });
    }
}

/**
 * Load example data
 */
function loadExampleData() {
    // Generate example Cq data
    currentCqData = [
        { Position: 'A1', Gene: 'GAPDH', Cq: 18.5 },
        { Position: 'A2', Gene: 'GAPDH', Cq: 18.6 },
        { Position: 'A3', Gene: 'GAPDH', Cq: 18.4 },
        { Position: 'B1', Gene: 'Target1', Cq: 22.3 },
        { Position: 'B2', Gene: 'Target1', Cq: 22.5 },
        { Position: 'B3', Gene: 'Target1', Cq: 22.4 },
        { Position: 'C1', Gene: 'Target2', Cq: 25.1 },
        { Position: 'C2', Gene: 'Target2', Cq: 25.3 },
        { Position: 'C3', Gene: 'Target2', Cq: 25.2 }
    ];
    currentCqData.columns = ['Position', 'Gene', 'Cq'];

    // Generate example design data
    currentDesignData = [
        { Position: 'A1', Group: 'Control', BioRep: '1' },
        { Position: 'A2', Group: 'Control', BioRep: '2' },
        { Position: 'A3', Group: 'Control', BioRep: '3' },
        { Position: 'B1', Group: 'Treatment', BioRep: '1' },
        { Position: 'B2', Group: 'Treatment', BioRep: '2' },
        { Position: 'B3', Group: 'Treatment', BioRep: '3' },
        { Position: 'C1', Group: 'Treatment', BioRep: '1' },
        { Position: 'C2', Group: 'Treatment', BioRep: '2' },
        { Position: 'C3', Group: 'Treatment', BioRep: '3' }
    ];
    currentDesignData.columns = ['Position', 'Group', 'BioRep'];

    // Show preview
    displayCqPreview(currentCqData);
    displayDesignPreview(currentDesignData);

    // Enable proceed button
    document.getElementById('proceedToAnalysis').disabled = false;

    // Navigate to import page
    navigateToPage('import');

    showNotification(i18n.t('msg.dataLoaded'), 'success');
}

/**
 * Load example Cq data only
 */
function loadExampleCqData() {
    // Clear existing data first
    currentCqData = null;

    // Generate example Cq data - properly paired for ΔΔCt calculation
    // Each position has one gene measurement
    currentCqData = [
        // Target gene: fos-glo-myc
        { Position: 'A1', Gene: 'fos-glo-myc', Cq: 22.3 },
        { Position: 'B1', Gene: 'fos-glo-myc', Cq: 22.0 },
        { Position: 'C1', Gene: 'fos-glo-myc', Cq: 21.5 },
        { Position: 'D1', Gene: 'fos-glo-myc', Cq: 19.8 },
        { Position: 'E1', Gene: 'fos-glo-myc', Cq: 20.2 },
        { Position: 'F1', Gene: 'fos-glo-myc', Cq: 20.0 },
        { Position: 'G1', Gene: 'fos-glo-myc', Cq: 19.5 },
        { Position: 'H1', Gene: 'fos-glo-myc', Cq: 18.9 },
        { Position: 'A2', Gene: 'fos-glo-myc', Cq: 19.2 },
        // Reference gene: Beta Actin
        { Position: 'A5', Gene: 'Beta Actin', Cq: 22.9 },
        { Position: 'B5', Gene: 'Beta Actin', Cq: 22.3 },
        { Position: 'C5', Gene: 'Beta Actin', Cq: 22.4 },
        { Position: 'D5', Gene: 'Beta Actin', Cq: 21.2 },
        { Position: 'E5', Gene: 'Beta Actin', Cq: 21.7 },
        { Position: 'F5', Gene: 'Beta Actin', Cq: 21.7 },
        { Position: 'G5', Gene: 'Beta Actin', Cq: 21.2 },
        { Position: 'H5', Gene: 'Beta Actin', Cq: 21.2 },
        { Position: 'A6', Gene: 'Beta Actin', Cq: 21.3 }
    ];

    console.log('Loaded example Cq data:', currentCqData.length, 'rows');
    console.log('First 6 rows of Cq data:', currentCqData.slice(0, 6));
    console.log('Full Cq data:', currentCqData);

    // Show preview
    displayCqPreview(currentCqData);

    // Check if both files are loaded
    checkDataLoaded();

    showNotification(i18n.t('msg.dataLoaded') + ' (Cq)', 'success');
}

/**
 * Load example Design data only
 */
function loadExampleDesignData() {
    // Generate example design data - matched to Cq data
    // Each BioRep has both fos-glo-myc and Beta Actin measured
    currentDesignData = [
        // Treatment group 0 - 3 biological replicates
        { Position: 'A1', Group: '0', BioRep: '1' },
        { Position: 'A5', Group: '0', BioRep: '1' },
        { Position: 'B1', Group: '0', BioRep: '2' },
        { Position: 'B5', Group: '0', BioRep: '2' },
        { Position: 'C1', Group: '0', BioRep: '3' },
        { Position: 'C5', Group: '0', BioRep: '3' },
        // Treatment group 0.5 - 3 biological replicates
        { Position: 'D1', Group: '0.5', BioRep: '1' },
        { Position: 'D5', Group: '0.5', BioRep: '1' },
        { Position: 'E1', Group: '0.5', BioRep: '2' },
        { Position: 'E5', Group: '0.5', BioRep: '2' },
        { Position: 'F1', Group: '0.5', BioRep: '3' },
        { Position: 'F5', Group: '0.5', BioRep: '3' },
        // Treatment group 1 - 3 biological replicates
        { Position: 'G1', Group: '1', BioRep: '1' },
        { Position: 'G5', Group: '1', BioRep: '1' },
        { Position: 'H1', Group: '1', BioRep: '2' },
        { Position: 'H5', Group: '1', BioRep: '2' },
        { Position: 'A2', Group: '1', BioRep: '3' },
        { Position: 'A6', Group: '1', BioRep: '3' }
    ];

    console.log('Loaded example Design data:', currentDesignData.length, 'rows');

    // Show preview
    displayDesignPreview(currentDesignData);

    // Check if both files are loaded
    checkDataLoaded();

    showNotification(i18n.t('msg.dataLoaded') + ' (Design)', 'success');
}

/**
 * Setup navigation between pages
 */
function setupNavigation() {
    document.querySelectorAll('[data-page]').forEach(element => {
        element.addEventListener('click', function(e) {
            e.preventDefault();
            const pageName = this.getAttribute('data-page');
            navigateToPage(pageName);
        });
    });
}

/**
 * Navigate to a specific page
 */
function navigateToPage(pageName) {
    // Hide all pages
    document.querySelectorAll('.page').forEach(page => {
        page.classList.remove('active');
        page.classList.add('d-none');
    });

    // Show target page
    const targetPage = document.getElementById(pageName + 'Page');
    if (targetPage) {
        targetPage.classList.remove('d-none');
        targetPage.classList.add('active');
    }

    // Update nav link states
    document.querySelectorAll('.nav-link').forEach(link => {
        link.classList.remove('active');
    });
    document.querySelector(`[data-page="${pageName}"]`).classList.add('active');
}

/**
 * Setup import page functionality
 */
function setupImportPage() {
    // Cq file loading
    document.getElementById('loadCqBtn').addEventListener('click', async function() {
        const fileInput = document.getElementById('cqFileInput');
        if (fileInput.files.length === 0) {
            showNotification(i18n.t('msg.noFileSelected'), 'warning');
            return;
        }

        const file = fileInput.files[0];
        const fileName = file && file.name ? file.name.toLowerCase() : '';
        if (fileName && !fileName.endsWith('.csv')) {
            const lang = i18n.getLanguage ? i18n.getLanguage() : 'zh';
            const msgZh = '仅支持 CSV 导入。Excel 请使用菜单 `File` -> `Open Cq File...` / `Open Design File...`。';
            const msgEn = 'Only CSV import is supported here. To import Excel, use `File` -> `Open Cq File...` / `Open Design File...`.';
            showNotification(lang === 'zh' ? msgZh : msgEn, 'warning');
            return;
        }
        console.log('=== Loading Cq file ===');
        console.log('File name:', file.name);
        console.log('File size:', file.size);
        console.log('File type:', file.type);

        if (bridge) {
            // Read file content and send to C++
            const reader = new FileReader();
            reader.onload = async function(e) {
                const content = e.target.result;
                console.log('File content length:', content.length);
                console.log('First 200 chars:', content.substring(0, 200));

                // Call C++ method to parse CSV content
                try {
                    console.log('Calling bridge.loadCqFromContent...');
                    const result = await bridge.loadCqFromContent(content);
                    console.log('Raw result from C++:', result);
                    console.log('Result type:', typeof result);
                    console.log('Result length:', result ? result.length : 0);

                    const parsed = JSON.parse(result);
                    console.log('Parsed result:', parsed);
                    console.log('Parsed result keys:', Object.keys(parsed));
                    console.log('parsed.data:', parsed.data);
                    console.log('parsed.data type:', Array.isArray(parsed.data) ? 'array' : typeof parsed.data);
                    console.log('parsed.columns:', parsed.columns);

                    if (!Array.isArray(parsed.data)) {
                        console.error('parsed.data is not an array! Full parsed object:', JSON.stringify(parsed, null, 2));
                        showNotification('Invalid data format: data is not an array', 'danger');
                        return;
                    }

                    currentCqData = parsed.data;
                    currentCqData.columns = parsed.columns;
                    console.log('Final currentCqData type:', Array.isArray(currentCqData) ? 'array' : typeof currentCqData);
                    console.log('Final currentCqData.columns:', currentCqData.columns);

                    displayCqPreview(currentCqData);
                    checkDataLoaded();
                    showNotification(i18n.t('msg.dataLoaded'), 'success');
                } catch (error) {
                    console.error('Error loading Cq file:', error);
                    console.error('Error stack:', error.stack);
                    showNotification('Failed to parse file: ' + error.message, 'danger');
                }
            };
            reader.onerror = function(e) {
                console.error('FileReader error:', e);
                showNotification('Failed to read file', 'danger');
            };
            reader.readAsText(file);
        } else {
            // Demo mode: load in browser
            loadCqFile(file);
        }
    });

    // Cq example data loading
    document.getElementById('loadCqExampleBtn').addEventListener('click', function() {
        loadExampleCqData();
    });

    // Design file loading
    document.getElementById('loadDesignBtn').addEventListener('click', async function() {
        const fileInput = document.getElementById('designFileInput');
        if (fileInput.files.length === 0) {
            showNotification(i18n.t('msg.noFileSelected'), 'warning');
            return;
        }

        const file = fileInput.files[0];
        const fileName = file && file.name ? file.name.toLowerCase() : '';
        if (fileName && !fileName.endsWith('.csv')) {
            const lang = i18n.getLanguage ? i18n.getLanguage() : 'zh';
            const msgZh = '仅支持 CSV 导入。Excel 请使用菜单 `File` -> `Open Cq File...` / `Open Design File...`。';
            const msgEn = 'Only CSV import is supported here. To import Excel, use `File` -> `Open Cq File...` / `Open Design File...`.';
            showNotification(lang === 'zh' ? msgZh : msgEn, 'warning');
            return;
        }
        console.log('=== Loading Design file ===');
        console.log('File name:', file.name);
        console.log('File size:', file.size);
        console.log('File type:', file.type);

        if (bridge) {
            // Read file content and send to C++
            const reader = new FileReader();
            reader.onload = async function(e) {
                const content = e.target.result;
                console.log('File content length:', content.length);
                console.log('First 200 chars:', content.substring(0, 200));

                // Call C++ method to parse CSV content
                try {
                    console.log('Calling bridge.loadDesignFromContent...');
                    const result = await bridge.loadDesignFromContent(content);
                    console.log('Raw result from C++:', result);
                    console.log('Result type:', typeof result);
                    console.log('Result length:', result ? result.length : 0);

                    const parsed = JSON.parse(result);
                    console.log('Parsed result keys:', Object.keys(parsed));
                    console.log('parsed.data type:', Array.isArray(parsed.data) ? 'array' : typeof parsed.data);
                    console.log('parsed.columns:', parsed.columns);

                    if (!Array.isArray(parsed.data)) {
                        console.error('parsed.data is not an array!');
                        showNotification('Invalid data format: data is not an array', 'danger');
                        return;
                    }

                    currentDesignData = parsed.data;
                    currentDesignData.columns = parsed.columns;
                    console.log('Final currentDesignData.columns:', currentDesignData.columns);

                    displayDesignPreview(currentDesignData);
                    checkDataLoaded();
                    showNotification(i18n.t('msg.dataLoaded'), 'success');
                } catch (error) {
                    console.error('Error loading Design file:', error);
                    console.error('Error stack:', error.stack);
                    showNotification('Failed to parse file: ' + error.message, 'danger');
                }
            };
            reader.onerror = function(e) {
                console.error('FileReader error:', e);
                showNotification('Failed to read file', 'danger');
            };
            reader.readAsText(file);
        } else {
            // Demo mode: load in browser
            loadDesignFile(file);
        }
    });

    // Design example data loading
    document.getElementById('loadDesignExampleBtn').addEventListener('click', function() {
        loadExampleDesignData();
    });

    // Proceed to analysis button
    document.getElementById('proceedToAnalysis').addEventListener('click', function() {
        if (currentCqData && currentDesignData) {
            navigateToPage('analysis');
        }
    });
}

/**
 * Load Cq data file
 */
function loadCqFile(file) {
    if (bridge) {
        // Use Qt file dialog
        bridge.showFileDialog(i18n.t('import.cqData'), '*.csv;;*.xlsx *.xls').then(filePath => {
            if (filePath) {
                try {
                    const result = bridge.loadCqFile(filePath);
                    console.log('=== Cq File Loading ===');
                    console.log('Raw result type:', typeof result);
                    console.log('Raw result length:', result.length);
                    console.log('Raw result (first 1000 chars):', result.substring(0, 1000));
                    const parsed = JSON.parse(result);
                    console.log('Parsed result keys:', Object.keys(parsed));
                    console.log('parsed.data:', parsed.data);
                    console.log('parsed.data type:', Array.isArray(parsed.data) ? 'array' : typeof parsed.data);
                    console.log('parsed.columns:', parsed.columns);
                    console.log('parsed.columns type:', Array.isArray(parsed.columns) ? 'array' : typeof parsed.columns);

                    if (!Array.isArray(parsed.data)) {
                        console.error('parsed.data is not an array! Full parsed object:', JSON.stringify(parsed, null, 2));
                        showNotification('Invalid data format: data is not an array', 'danger');
                        return;
                    }

                    currentCqData = parsed.data;
                    currentCqData.columns = parsed.columns;
                    displayCqPreview(currentCqData);
                    checkDataLoaded();
                } catch (error) {
                    console.error('Error loading Cq file:', error);
                    console.error('Error stack:', error.stack);
                    showNotification(i18n.t('msg.error') + ': ' + error.message, 'danger');
                }
            }
        });
    } else {
        // Demo mode: parse file in browser
        const reader = new FileReader();
        reader.onload = function(e) {
            try {
                const data = parseCSV(e.target.result);
                currentCqData = data;
                displayCqPreview(data);
                checkDataLoaded();
                showNotification(i18n.t('msg.dataLoaded'), 'success');
            } catch (error) {
                showNotification(i18n.t('msg.error') + ': ' + error.message, 'danger');
            }
        };
        reader.readAsText(file);
    }
}

/**
 * Load design data file
 */
function loadDesignFile(file) {
    if (bridge) {
        bridge.showFileDialog(i18n.t('import.designData'), '*.csv;;*.xlsx *.xls').then(filePath => {
            if (filePath) {
                const result = bridge.loadDesignFile(filePath);
                const parsed = JSON.parse(result);
                currentDesignData = parsed.data;
                currentDesignData.columns = parsed.columns;
                displayDesignPreview(currentDesignData);
                checkDataLoaded();
            }
        });
    } else {
        // Demo mode
        const reader = new FileReader();
        reader.onload = function(e) {
            try {
                const data = parseCSV(e.target.result);
                currentDesignData = data;
                displayDesignPreview(data);
                checkDataLoaded();
                showNotification(i18n.t('msg.dataLoaded'), 'success');
            } catch (error) {
                showNotification(i18n.t('msg.error') + ': ' + error.message, 'danger');
            }
        };
        reader.readAsText(file);
    }
}

/**
 * Display Cq data preview
 */
function displayCqPreview(data) {
    console.log('=== displayCqPreview ===');
    console.log('Input data:', data);
    console.log('Data type:', Array.isArray(data) ? 'array' : typeof data);
    console.log('Data length:', data ? data.length : 0);

    const table = document.getElementById('cqPreviewTable');
    const thead = table.querySelector('thead tr');
    const tbody = table.querySelector('tbody');

    if (!data) {
        console.error('No data provided to displayCqPreview');
        tbody.innerHTML = '<tr><td class="text-muted">No data</td></tr>';
        return;
    }

    if (!Array.isArray(data)) {
        console.error('Data is not an array:', typeof data, data);
        tbody.innerHTML = '<tr><td class="text-muted">Invalid data format</td></tr>';
        return;
    }

    if (data.length === 0) {
        console.warn('Data array is empty');
        tbody.innerHTML = '<tr><td class="text-muted">' + i18n.t('table.noData') + '</td></tr>';
        return;
    }

    console.log('First row:', data[0]);

    // Clear existing
    thead.innerHTML = '';
    tbody.innerHTML = '';

    // Headers - 优先使用保存的列顺序
    const headers = data.columns || Object.keys(data[0]);
    console.log('Headers:', headers);
    headers.forEach(header => {
        const th = document.createElement('th');
        th.textContent = header;
        thead.appendChild(th);
    });

    // Rows (limit to first 10)
    const displayRows = data.slice(0, 10);
    console.log('Displaying', displayRows.length, 'rows');
    displayRows.forEach(row => {
        const tr = document.createElement('tr');
        headers.forEach(header => {
            const td = document.createElement('td');
            td.textContent = row[header];
            tr.appendChild(td);
        });
        tbody.appendChild(tr);
    });

    console.log('Table rendered successfully');
}

/**
 * Display design data preview
 */
function displayDesignPreview(data) {
    console.log('=== displayDesignPreview ===');
    console.log('Input data:', data);
    console.log('Data type:', Array.isArray(data) ? 'array' : typeof data);
    console.log('Data length:', data ? data.length : 0);

    const table = document.getElementById('designPreviewTable');
    const thead = table.querySelector('thead tr');
    const tbody = table.querySelector('tbody');

    if (!data) {
        console.error('No data provided to displayDesignPreview');
        tbody.innerHTML = '<tr><td class="text-muted">No data</td></tr>';
        return;
    }

    if (!Array.isArray(data)) {
        console.error('Data is not an array:', typeof data, data);
        tbody.innerHTML = '<tr><td class="text-muted">Invalid data format</td></tr>';
        return;
    }

    if (data.length === 0) {
        console.warn('Data array is empty');
        tbody.innerHTML = '<tr><td class="text-muted">' + i18n.t('table.noData') + '</td></tr>';
        return;
    }

    console.log('First row:', data[0]);

    thead.innerHTML = '';
    tbody.innerHTML = '';

    // Headers - 优先使用保存的列顺序
    const headers = data.columns || Object.keys(data[0]);
    console.log('Headers:', headers);
    headers.forEach(header => {
        const th = document.createElement('th');
        th.textContent = header;
        thead.appendChild(th);
    });

    const displayRows = data.slice(0, 10);
    console.log('Displaying', displayRows.length, 'rows');
    displayRows.forEach(row => {
        const tr = document.createElement('tr');
        headers.forEach(header => {
            const td = document.createElement('td');
            td.textContent = row[header];
            tr.appendChild(td);
        });
        tbody.appendChild(tr);
    });

    console.log('Table rendered successfully');
}

/**
 * Check if both files are loaded
 */
function checkDataLoaded() {
    const proceedBtn = document.getElementById('proceedToAnalysis');

    console.log('=== checkDataLoaded ===');
    console.log('currentCqData:', currentCqData);
    console.log('currentCqData type:', Array.isArray(currentCqData) ? 'array' : typeof currentCqData);
    console.log('currentCqData length:', currentCqData ? currentCqData.length : 0);
    console.log('currentDesignData:', currentDesignData);
    console.log('currentDesignData type:', Array.isArray(currentDesignData) ? 'array' : typeof currentDesignData);
    console.log('currentDesignData length:', currentDesignData ? currentDesignData.length : 0);

    if (currentCqData && currentDesignData) {
        console.log('Both datasets loaded, enabling proceed button');
        proceedBtn.disabled = false;

        // Update dropdown options
        updateParameterDropdowns();
    } else {
        console.log('Missing data, proceed button remains disabled');
        if (!currentCqData) console.log('  - currentCqData is missing');
        if (!currentDesignData) console.log('  - currentDesignData is missing');
    }
}

/**
 * Update parameter dropdowns based on loaded data
 */
function updateParameterDropdowns() {
    try {
        // Update reference gene dropdown from Cq data
        const referenceGeneSelect = document.getElementById('referenceGene');
        if (referenceGeneSelect && currentCqData && Array.isArray(currentCqData)) {
            // Extract unique genes
            const genes = new Set();
            currentCqData.forEach(row => {
                if (row.Gene) {
                    genes.add(row.Gene);
                }
            });

            // Save current selection
            const currentValue = referenceGeneSelect.value;

            // Clear existing options (keep the first default option)
            referenceGeneSelect.innerHTML = '<option value="">-- Select Reference Gene --</option>';

            // Add new options
            genes.forEach(gene => {
                const option = document.createElement('option');
                option.value = gene;
                option.textContent = gene;
                referenceGeneSelect.appendChild(option);
            });

            // Restore selection if it still exists
            if (genes.has(currentValue)) {
                referenceGeneSelect.value = currentValue;
            }

            console.log('Updated reference gene dropdown with', genes.size, 'unique genes');
        }

        // Update control group dropdown from Design data
        const controlGroupSelect = document.getElementById('controlGroup');
        if (controlGroupSelect && currentDesignData && Array.isArray(currentDesignData)) {
            // Extract unique groups
            const groups = new Set();
            currentDesignData.forEach(row => {
                if (row.Group) {
                    groups.add(row.Group);
                }
            });

            // Save current selection
            const currentValue = controlGroupSelect.value;

            // Clear existing options (keep the first default option)
            controlGroupSelect.innerHTML = '<option value="">-- Select Control Group --</option>';

            // Add new options
            groups.forEach(group => {
                const option = document.createElement('option');
                option.value = group;
                option.textContent = group;
                controlGroupSelect.appendChild(option);
            });

            // Restore selection if it still exists
            if (groups.has(currentValue)) {
                controlGroupSelect.value = currentValue;
            }

            console.log('Updated control group dropdown with', groups.size, 'unique groups');
        }
    } catch (error) {
        console.error('Error updating parameter dropdowns:', error);
    }
}

/**
 * Setup analysis page
 */
function setupAnalysisPage() {
    document.getElementById('runAnalysis').addEventListener('click', function() {
        runAnalysis();
    });

    // Add event listeners for method selection
    const methodInputs = document.querySelectorAll('input[name="analysisMethod"]');
    methodInputs.forEach(input => {
        input.addEventListener('change', function() {
            updateAnalysisMethodUI();
        });
    });

    // Initialize UI state
    updateAnalysisMethodUI();
}

/**
 * Update UI based on selected analysis method
 */
function updateAnalysisMethodUI() {
    const method = document.querySelector('input[name="analysisMethod"]:checked').value;
    const controlGroupField = document.getElementById('controlGroup').closest('.mb-3');
    const statisticalTestField = document.getElementById('statisticalTest').closest('.mb-3');

    if (method === 'deltaCt') {
        // Hide control group for ΔCt method, but show statistical test
        controlGroupField.style.display = 'none';
        statisticalTestField.style.display = 'block';
    } else if (method === 'deltaDeltaCt') {
        // Show both for ΔΔCt method
        controlGroupField.style.display = 'block';
        statisticalTestField.style.display = 'block';
    } else {
        // For future methods (e.g., Standard Curve)
        controlGroupField.style.display = 'none';
        statisticalTestField.style.display = 'block';
    }
}

/**
 * Run analysis
 */
async function runAnalysis() {
    const method = document.querySelector('input[name="analysisMethod"]:checked').value;
    const referenceGene = document.getElementById('referenceGene').value.trim();
    const controlGroup = document.getElementById('controlGroup').value.trim();
    const statisticalTest = document.getElementById('statisticalTest').value;
    const removeOutliers = document.getElementById('removeOutliers').checked;
    const colorPalette = document.getElementById('colorPalette').value;

    console.log('=== Analysis Parameters ===');
    console.log('Method:', method);
    console.log('Reference Gene:', referenceGene);
    console.log('Control Group:', controlGroup);
    console.log('Statistical Test:', statisticalTest);
    console.log('Remove Outliers:', removeOutliers);
    console.log('Color Palette:', colorPalette);

    // Validation
    if (!referenceGene) {
        showNotification(i18n.t('msg.missingParams') + ': ' + i18n.t('param.referenceGene'), 'warning');
        return;
    }

    if (method === 'deltaDeltaCt' && !controlGroup) {
        showNotification(i18n.t('msg.missingParams') + ': ' + i18n.t('param.controlGroup'), 'warning');
        return;
    }

    // Check if data is loaded
    if (!currentCqData || currentCqData.length === 0) {
        showNotification('No Cq data loaded', 'danger');
        return;
    }

    if (!currentDesignData || currentDesignData.length === 0) {
        showNotification('No design data loaded', 'danger');
        return;
    }

    const params = {
        referenceGene: referenceGene,
        controlGroup: controlGroup,
        removeOutliers: removeOutliers,
        colorPalette: colorPalette
    };

    if (bridge) {
        try {
            // Send data to C++ backend first
            console.log('=== Sending data to C++ ===');
            console.log('Cq data type:', Array.isArray(currentCqData) ? 'array' : typeof currentCqData);
            console.log('Cq data length:', currentCqData ? currentCqData.length : 0);
            console.log('Cq data sample:', currentCqData ? currentCqData.slice(0, 6) : 'null');
            console.log('Design data type:', Array.isArray(currentDesignData) ? 'array' : typeof currentDesignData);
            console.log('Design data length:', currentDesignData ? currentDesignData.length : 0);
            console.log('Design data sample:', currentDesignData ? currentDesignData.slice(0, 6) : 'null');

            const cqJson = JSON.stringify(currentCqData);
            const designJson = JSON.stringify(currentDesignData);
            console.log('Cq JSON length:', cqJson.length);
            console.log('Design JSON length:', designJson.length);
            console.log('Cq JSON (first 500 chars):', cqJson.substring(0, 500));
            console.log('Design JSON (first 300 chars):', designJson.substring(0, 300));

            await bridge.setCqData(cqJson);
            await bridge.setDesignData(designJson);

            console.log('Data sent to C++ successfully');

            // Call C++ backend based on selected method
            let result;
            if (method === 'standardCurve') {
                result = await bridge.calculateByStandardCurve(JSON.stringify(params), statisticalTest);
            } else if (method === 'deltaCt') {
                result = await bridge.calculateByDeltaCt(JSON.stringify(params), statisticalTest);
            } else { // deltaDeltaCt
                result = await bridge.calculateByDeltaDeltaCt(JSON.stringify(params), statisticalTest);
            }

            console.log('Raw result from C++:', result);
            analysisResults = JSON.parse(result);
            console.log('Parsed analysisResults:', analysisResults);
            console.log('Table data:', analysisResults.table);
            console.log('Table length:', analysisResults.table ? analysisResults.table.length : 0);
            console.log('Statistics:', analysisResults.statistics);

            // Navigate to results page first
            navigateToPage('results');

            // Then display results with a small delay to ensure DOM is ready
            setTimeout(() => {
                displayResults(analysisResults);
                showNotification(i18n.t('msg.analysisCompleted'), 'success');
            }, 100);
        } catch (error) {
            showNotification('Analysis failed: ' + error.message, 'danger');
        }
    } else {
        // Demo mode: generate mock results
        analysisResults = generateMockResults();

        // Navigate to results page first
        navigateToPage('results');

        // Then display results with a small delay to ensure DOM is ready
        setTimeout(() => {
            displayResults(analysisResults);
            showNotification(i18n.t('msg.analysisCompleted'), 'success');
        }, 100);
    }
}

/**
 * Display analysis results
 */
function displayResults(results) {
    displayResultsTable(results);
    displayRawData(results);
    // displayCharts(results);  // Charts feature temporarily disabled
    // Statistics are now integrated into the main table
}

/**
 * Display results table
 */
function displayResultsTable(results) {
    console.log('displayResultsTable called with:', results);

    const table = document.getElementById('resultsTable');
    const thead = table.querySelector('thead');
    const tbody = table.querySelector('tbody');

    if (!results) {
        console.error('No results provided');
        tbody.innerHTML = '<tr><td colspan="5">No results</td></tr>';
        return;
    }

    if (!results.table) {
        console.error('No table in results:', results);
        tbody.innerHTML = '<tr><td colspan="5">No table data</td></tr>';
        return;
    }

    if (!Array.isArray(results.table)) {
        console.error('Table is not an array:', typeof results.table, results.table);
        tbody.innerHTML = '<tr><td colspan="5">Invalid table format</td></tr>';
        return;
    }

    if (results.table.length === 0) {
        console.warn('Table is empty');
        tbody.innerHTML = '<tr><td colspan="5">' + i18n.t('table.noData') + '</td></tr>';
        return;
    }

    console.log('Table has', results.table.length, 'rows');
    console.log('First row:', results.table[0]);

    thead.innerHTML = '';
    tbody.innerHTML = '';

    // Define the desired column order
    const headers = ['Gene', 'Group', 'Mean', 'StdDev', 'PValue', 'Significance'];
    console.log('Headers:', headers);

    const headerRow = document.createElement('tr');
    headers.forEach(header => {
        const th = document.createElement('th');
        th.textContent = header;
        headerRow.appendChild(th);
    });
    thead.appendChild(headerRow);

    results.table.forEach(row => {
        const tr = document.createElement('tr');
        headers.forEach(header => {
            const td = document.createElement('td');
            const value = row[header];
            // Format numeric values
            if (typeof value === 'number') {
                td.textContent = value.toFixed(4);
            } else {
                td.textContent = value || '';
            }
            tr.appendChild(td);
        });
        tbody.appendChild(tr);
    });

    console.log('Table rendered with', tbody.children.length, 'rows');
}

/**
 * Display raw data table (BioRep level data)
 */
function displayRawData(results) {
    console.log('displayRawData called with:', results);

    const table = document.getElementById('rawDataTable');
    const thead = table.querySelector('thead');
    const tbody = table.querySelector('tbody');

    if (!results) {
        console.error('No results provided');
        tbody.innerHTML = '<tr><td colspan="6">No results</td></tr>';
        return;
    }

    if (!results.rawData) {
        console.error('No rawData in results:', results);
        tbody.innerHTML = '<tr><td colspan="6">No raw data</td></tr>';
        return;
    }

    if (!Array.isArray(results.rawData)) {
        console.error('RawData is not an array:', typeof results.rawData, results.rawData);
        tbody.innerHTML = '<tr><td colspan="6">Invalid raw data format</td></tr>';
        return;
    }

    if (results.rawData.length === 0) {
        console.warn('Raw data is empty');
        tbody.innerHTML = '<tr><td colspan="6">No raw data</td></tr>';
        return;
    }

    console.log('Raw data has', results.rawData.length, 'rows');
    console.log('First raw row:', results.rawData[0]);

    thead.innerHTML = '';
    tbody.innerHTML = '';

    // Define the desired column order
    const headers = ['Gene', 'Group', 'BioRep', 'Expression', 'Mean', 'SD'];
    console.log('Raw data headers:', headers);

    const headerRow = document.createElement('tr');
    headers.forEach(header => {
        const th = document.createElement('th');
        th.textContent = header;
        headerRow.appendChild(th);
    });
    thead.appendChild(headerRow);

    results.rawData.forEach(row => {
        const tr = document.createElement('tr');
        headers.forEach(header => {
            const td = document.createElement('td');
            const value = row[header];
            // Format numeric values
            if (typeof value === 'number') {
                td.textContent = value.toFixed(4);
            } else {
                td.textContent = value || '';
            }
            tr.appendChild(td);
        });
        tbody.appendChild(tr);
    });

    console.log('Raw data table rendered with', tbody.children.length, 'rows');
}

/**
 * Display charts using ECharts
 */
function displayCharts(results) {
    const chartDom = document.getElementById('mainChart');

    if (mainChart) {
        mainChart.dispose();
    }

    mainChart = echarts.init(chartDom);

    const palette = colorPalettes[document.getElementById('colorPalette').value] || colorPalettes.nature;

    const option = {
        title: {
            text: 'Gene Expression Analysis',
            left: 'center',
            textStyle: {
                fontSize: 18,
                fontWeight: 'bold'
            }
        },
        tooltip: {
            trigger: 'axis',
            axisPointer: {
                type: 'shadow'
            }
        },
        legend: {
            top: 30,
            left: 'center'
        },
        grid: {
            left: '3%',
            right: '4%',
            bottom: '15%',
            top: '20%',
            containLabel: true
        },
        xAxis: {
            type: 'category',
            data: ['Gene1', 'Gene2', 'Gene3'],
            axisLabel: {
                rotate: 45,
                fontSize: 12
            }
        },
        yAxis: {
            type: 'value',
            name: 'Relative Expression',
            axisLabel: {
                fontSize: 12
            }
        },
        series: [
            {
                name: 'Control',
                type: 'bar',
                data: [1.0, 1.0, 1.0],
                itemStyle: {
                    color: palette[0]
                }
            },
            {
                name: 'Treatment',
                type: 'bar',
                data: [2.5, 1.8, 3.2],
                itemStyle: {
                    color: palette[1]
                }
            }
        ]
    };

    mainChart.setOption(option);

    // Responsive
    window.addEventListener('resize', function() {
        mainChart.resize();
    });
}

/**
 * Setup results page
 */
function setupResultsPage() {
    // Summary data export buttons
    document.getElementById('exportCsv').addEventListener('click', function() {
        exportResults('csv', 'summary');
    });

    document.getElementById('exportExcel').addEventListener('click', function() {
        exportResults('excel', 'summary');
    });

    // Raw data export buttons
    document.getElementById('exportRawCsv').addEventListener('click', function() {
        exportResults('csv', 'raw');
    });

    document.getElementById('exportRawExcel').addEventListener('click', function() {
        exportResults('excel', 'raw');
    });

    // Chart export feature temporarily disabled
    // document.getElementById('exportChart').addEventListener('click', function() {
    //     exportChart();
    // });
}

/**
 * Export results
 * @param {string} format - 'csv' or 'excel'
 * @param {string} dataType - 'summary' or 'raw'
 */
function exportResults(format, dataType) {
    if (!analysisResults) {
        showNotification(i18n.t('msg.noData'), 'warning');
        return;
    }

    // Check if the requested data type exists
    if (dataType === 'raw' && (!analysisResults.rawData || !Array.isArray(analysisResults.rawData) || analysisResults.rawData.length === 0)) {
        showNotification('No raw data available', 'warning');
        return;
    }

    if (dataType === 'summary' && (!analysisResults.table || !Array.isArray(analysisResults.table) || analysisResults.table.length === 0)) {
        showNotification('No summary data available', 'warning');
        return;
    }

    // Prepare data for export
    const exportData = {
        table: dataType === 'raw' ? analysisResults.rawData : analysisResults.table
    };

    if (bridge) {
        const filter = format === 'csv' ? '*.csv' : '*.xlsx';
        const defaultName = dataType === 'raw' ? 'raw_data' : 'summary_data';
        bridge.showSaveDialog(i18n.t('btn.export'), filter, defaultName).then(filePath => {
            if (filePath) {
                if (format === 'csv') {
                    bridge.exportToCSV(JSON.stringify(exportData), filePath);
                } else {
                    bridge.exportToExcel(JSON.stringify(exportData), filePath);
                }
                showNotification(i18n.t('msg.analysisCompleted'), 'success');
            }
        });
    } else {
        // Demo mode: download as CSV
        const csv = convertToCSV(exportData.table);
        const filename = dataType === 'raw' ? 'raw_data.csv' : 'results.csv';
        downloadFile(csv, filename, 'text/csv');
        showNotification(i18n.t('msg.analysisCompleted'), 'success');
    }
}

/**
 * Export chart as image
 */
function exportChart() {
    if (!mainChart) {
        showNotification(i18n.t('msg.noData'), 'warning');
        return;
    }

    const url = mainChart.getDataURL({
        type: 'png',
        pixelRatio: 2,
        backgroundColor: '#fff'
    });

    const link = document.createElement('a');
    link.href = url;
    link.download = 'chart.png';
    link.click();
}

/**
 * Setup language switcher
 */
function setupLanguageSwitcher() {
    document.querySelectorAll('[data-lang]').forEach(element => {
        element.addEventListener('click', function(e) {
            e.preventDefault();
            const lang = this.getAttribute('data-lang');
            i18n.setLanguage(lang);

            if (bridge) {
                bridge.setLanguage(lang);
            }
        });
    });
}

/**
 * Show progress modal
 */
function showProgress(show, message = '') {
    const modal = new bootstrap.Modal(document.getElementById('progressModal'));
    const progressBar = document.getElementById('progressBar');
    const progressMessage = document.getElementById('progressMessage');

    progressMessage.textContent = message;

    if (show) {
        modal.show();
    } else {
        modal.hide();
    }
}

/**
 * Show notification toast
 */
function showNotification(message, type = 'info') {
    const toastEl = document.getElementById('toast');
    const toastTitle = document.getElementById('toastTitle');
    const toastBody = document.getElementById('toastBody');

    // Set color based on type
    let bgClass = 'bg-primary';
    if (type === 'success') bgClass = 'bg-success';
    if (type === 'danger' || type === 'error') bgClass = 'bg-danger';
    if (type === 'warning') bgClass = 'bg-warning';

    toastTitle.textContent = type.charAt(0).toUpperCase() + type.slice(1);
    toastBody.textContent = message;

    const toast = new bootstrap.Toast(toastEl, {
        delay: 1500,  // 1.5秒后自动关闭
        autohide: true
    });
    toast.show();
}

/**
 * Bridge signal handlers
 */
function onDataLoaded(success, message) {
    if (success) {
        showNotification(i18n.t('msg.dataLoaded') + ': ' + message, 'success');
    } else {
        showNotification(i18n.t('msg.error') + ': ' + message, 'danger');
    }
}

function onCalculationCompleted(success, message) {
    console.log('onCalculationCompleted called:', success, message);

    if (success) {
        showNotification(i18n.t('msg.analysisCompleted') + ': ' + message, 'success');
    } else {
        showNotification(i18n.t('msg.error') + ': ' + message, 'danger');
    }
}

function onErrorOccurred(error) {
    showNotification(error, 'danger');
}

function onProgressChanged(progress, message) {
    const progressBar = document.getElementById('progressBar');
    const progressMessage = document.getElementById('progressMessage');

    progressBar.style.width = progress + '%';
    progressMessage.textContent = message;
}

/**
 * Utility functions
 */
function parseCSV(text) {
    const lines = text.trim().split('\n');
    const headers = lines[0].split(',').map(h => h.trim());
    const data = [];

    for (let i = 1; i < lines.length; i++) {
        const values = lines[i].split(',').map(v => v.trim());
        const row = {};
        headers.forEach((header, index) => {
            row[header] = values[index];
        });
        data.push(row);
    }

    // 保存列顺序
    data.columns = headers;
    return data;
}

function convertToCSV(data) {
    if (!data || data.length === 0) return '';

    // 优先使用保存的列顺序
    const headers = data.columns || Object.keys(data[0]);
    const csvRows = [];

    csvRows.push(headers.join(','));

    data.forEach(row => {
        const values = headers.map(header => row[header]);
        csvRows.push(values.join(','));
    });

    return csvRows.join('\n');
}

function downloadFile(content, filename, type) {
    const blob = new Blob([content], { type: type });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = filename;
    link.click();
    URL.revokeObjectURL(url);
}

function generateMockResults() {
    return {
        method: 'ΔΔCt',
        table: [
            { Group: 'Control', Gene: 'Gene1', BioRep: '1', Expression: '1.00' },
            { Group: 'Control', Gene: 'Gene1', BioRep: '2', Expression: '1.10' },
            { Group: 'Control', Gene: 'Gene1', BioRep: '3', Expression: '0.95' },
            { Group: 'Treatment', Gene: 'Gene1', BioRep: '1', Expression: '2.50' },
            { Group: 'Treatment', Gene: 'Gene1', BioRep: '2', Expression: '2.80' },
            { Group: 'Treatment', Gene: 'Gene1', BioRep: '3', Expression: '2.30' },
            { Group: 'Control', Gene: 'Gene2', BioRep: '1', Expression: '1.00' },
            { Group: 'Control', Gene: 'Gene2', BioRep: '2', Expression: '1.05' },
            { Group: 'Control', Gene: 'Gene2', BioRep: '3', Expression: '0.98' },
            { Group: 'Treatment', Gene: 'Gene2', BioRep: '1', Expression: '1.80' },
            { Group: 'Treatment', Gene: 'Gene2', BioRep: '2', Expression: '1.75' },
            { Group: 'Treatment', Gene: 'Gene2', BioRep: '3', Expression: '1.85' }
        ],
        statistics: [
            { gene: 'Gene1', group1: 'Control', group2: 'Treatment', tStatistic: -12.5, pValue: 0.0002, significance: '***' },
            { gene: 'Gene2', group1: 'Control', group2: 'Treatment', tStatistic: -8.3, pValue: 0.001, significance: '**' }
        ]
    };
}

/**
 * Download Cq data template
 */
function downloadCqTemplate() {
    const csv = `Position,Gene,Cq
A1,GAPDH,18.5
A2,GAPDH,18.6
A3,GAPDH,18.4
B1,GAPDH,18.7
B2,GAPDH,18.5
B3,GAPDH,18.6
C1,Target1,22.3
C2,Target1,22.5
C3,Target1,22.4
D1,Target1,22.6
D2,Target1,22.3
D3,Target1,22.5
E1,Target2,25.1
E2,Target2,25.3
E3,Target2,25.2
F1,Target2,25.4
F2,Target2,25.1
F3,Target2,25.3`;

    downloadFile(csv, 'cq_template.csv', 'text/csv');
    showNotification('Cq模板下载成功！文件保存在：Downloads/cq_template.csv', 'success');
}

/**
 * Download experimental design template
 */
function downloadDesignTemplate() {
    const csv = `Position,Group,BioRep
A1,Control,1
A2,Control,2
A3,Control,3
B1,Control,1
B2,Control,2
B3,Control,3
C1,Treatment,1
C2,Treatment,2
C3,Treatment,3
D1,Treatment,1
D2,Treatment,2
D3,Treatment,3
E1,Treatment,1
E2,Treatment,2
E3,Treatment,3
F1,Treatment,1
F2,Treatment,2
F3,Treatment,3`;

    downloadFile(csv, 'design_template.csv', 'text/csv');
    showNotification('实验设计模板下载成功！文件保存在：Downloads/design_template.csv', 'success');
}

/**
 * Download complete example data files
 */
function downloadExampleDataFiles() {
    // Download Cq example
    const cqCsv = `Position,Gene,Cq
A1,GAPDH,18.5
A2,GAPDH,18.6
A3,GAPDH,18.4
B1,GAPDH,18.7
B2,GAPDH,18.5
B3,GAPDH,18.6
C1,Target1,22.3
C2,Target1,22.5
C3,Target1,22.4
D1,Target1,22.6
D2,Target1,22.3
D3,Target1,22.5
E1,Target2,25.1
E2,Target2,25.3
E3,Target2,25.2
F1,Target2,25.4
F2,Target2,25.1
F3,Target2,25.3`;

    downloadFile(cqCsv, 'example_cq_data.csv', 'text/csv');

    // Download Design example with a small delay
    setTimeout(() => {
        const designCsv = `Position,Group,BioRep
A1,Control,1
A2,Control,2
A3,Control,3
B1,Control,1
B2,Control,2
B3,Control,3
C1,Treatment,1
C2,Treatment,2
C3,Treatment,3
D1,Treatment,1
D2,Treatment,2
D3,Treatment,3
E1,Treatment,1
E2,Treatment,2
E3,Treatment,3
F1,Treatment,1
F2,Treatment,2
F3,Treatment,3`;

        downloadFile(designCsv, 'example_design_data.csv', 'text/csv');
        showNotification('示例数据下载完成！文件保存在：Downloads/example_cq_data.csv 和 example_design_data.csv', 'success');
    }, 500);
}

// Initialize when DOM is ready
document.addEventListener('DOMContentLoaded', function() {
    // Wait for WebChannel
    if (typeof qt !== 'undefined') {
        initializeApplication();
    } else {
        // For testing in regular browser
        console.log('Running in standalone mode');
        initializeUI();
    }
});

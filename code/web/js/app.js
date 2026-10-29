/**
 * qPCRtools Main Application
 * Handles UI interactions and C++ bridge communication
 */

// Global variables
let bridge = null;
let currentCqData = null;
let currentDesignData = null;
let currentConcenData = null;  // Concentration data for Standard Curve
let analysisResults = null;
// 输入文件名与分析参数快照：用于导出"分析记录"（可核查、可重跑）
let currentCqFileName = '';
let currentDesignFileName = '';
let currentConcenFileName = '';
let lastAnalysisParameters = null;
let mainChart = null;

// Standard Curve specific data
let scCqData = null;
let scConcenData = null;
let scResults = null;

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
    setupStandardCurvePage();

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
        // Skip placeholder cards
        if (card.classList.contains('placeholder-card')) {
            return;
        }

        card.style.cursor = 'pointer';
        card.addEventListener('click', function() {
            console.log('Feature card clicked!');
            const action = this.getAttribute('data-action');
            if (action === 'start-analysis') {
                navigateToPage('import');
            } else if (action === 'goto-standard-curve') {
                navigateToPage('standardCurve');
            } else if (action === 'goto-templates') {
                // Open templates modal
                const modal = new bootstrap.Modal(document.getElementById('templatesModal'));
                modal.show();
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

    // Design template with efficiency
    const modalDownloadDesignWithEffBtn = document.getElementById('modalDownloadDesignWithEffTemplate');
    if (modalDownloadDesignWithEffBtn) {
        modalDownloadDesignWithEffBtn.addEventListener('click', function(e) {
            e.preventDefault();
            downloadDesignWithEffTemplate();
        });
    }

    // Design Template with Slope/Intercept
    const modalDownloadDesignWithSlopeBtn = document.getElementById('modalDownloadDesignWithSlopeTemplate');
    if (modalDownloadDesignWithSlopeBtn) {
        modalDownloadDesignWithSlopeBtn.addEventListener('click', function(e) {
            e.preventDefault();
            downloadDesignWithSlopeTemplate();
        });
    }

    // Standard Curve Expression example
    const modalDownloadSCExpExampleBtn = document.getElementById('modalDownloadStandardCurveExpExample');
    if (modalDownloadSCExpExampleBtn) {
        modalDownloadSCExpExampleBtn.addEventListener('click', function(e) {
            e.preventDefault();
            downloadStandardCurveExpExample();
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
 * Load example Concentration data only
 */
function loadExampleConcenData() {
    // Clear existing data first
    currentConcenData = null;

    // Generate example concentration data for standard curve
    // Matches positions in the Cq data
    currentConcenData = [
        { Position: 'A1', Gene: 'fos-glo-myc', Conc: 4 },
        { Position: 'B1', Gene: 'fos-glo-myc', Conc: 16 },
        { Position: 'C1', Gene: 'fos-glo-myc', Conc: 64 },
        { Position: 'D1', Gene: 'fos-glo-myc', Conc: 256 },
        { Position: 'E1', Gene: 'fos-glo-myc', Conc: 1024 },
        { Position: 'F1', Gene: 'fos-glo-myc', Conc: 4096 },
        { Position: 'A5', Gene: 'Beta Actin', Conc: 4 },
        { Position: 'B5', Gene: 'Beta Actin', Conc: 16 },
        { Position: 'C5', Gene: 'Beta Actin', Conc: 64 },
        { Position: 'D5', Gene: 'Beta Actin', Conc: 256 },
        { Position: 'E5', Gene: 'Beta Actin', Conc: 1024 },
        { Position: 'F5', Gene: 'Beta Actin', Conc: 4096 }
    ];

    console.log('Loaded example Concentration data:', currentConcenData.length, 'rows');

    // Show preview
    displayConcenPreview(currentConcenData);

    // Check if both files are loaded
    checkDataLoaded();

    showNotification(i18n.t('msg.dataLoaded') + ' (Concentration)', 'success');
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
    // 无对应导航链接时不再抛 TypeError（如某些页面只在特定路径可达）
    const navLink = document.querySelector(`[data-page="${pageName}"]`);
    if (navLink) {
        navLink.classList.add('active');
    }
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
        const fileNameLower = file && file.name ? file.name.toLowerCase() : '';
        const isCsv = fileNameLower.endsWith('.csv');
        const isExcel = fileNameLower.endsWith('.xlsx') || fileNameLower.endsWith('.xls');
        if (!isCsv && !isExcel) {
            const lang = i18n.getLanguage ? i18n.getLanguage() : 'zh';
            const msgZh = '仅支持 CSV 或 Excel（.xlsx/.xls）导入。';
            const msgEn = 'Only CSV or Excel (.xlsx/.xls) import is supported here.';
            showNotification(lang === 'zh' ? msgZh : msgEn, 'warning');
            return;
        }
        console.log('=== Loading Cq file ===');
        console.log('File name:', file.name);
        currentCqFileName = file.name;
        console.log('File size:', file.size);
        console.log('File type:', file.type);

        if (bridge) {
            // Read file content and send to C++
            const reader = new FileReader();
            reader.onload = async function(e) {
                try {
                    if (isCsv) {
                        const content = e.target.result;
                        console.log('File content length:', content.length);
                        console.log('First 200 chars:', content.substring(0, 200));

                        console.log('Calling bridge.loadCqFromContent...');
                        const result = await bridge.loadCqFromContent(content);

                        const parsed = JSON.parse(result);
                        if (parsed && parsed.error) {
                            showNotification(parsed.error, 'danger');
                            return;
                        }

                        if (!Array.isArray(parsed.data)) {
                            console.error('parsed.data is not an array! Full parsed object:', JSON.stringify(parsed, null, 2));
                            showNotification('Invalid data format: data is not an array', 'danger');
                            return;
                        }

                        currentCqData = parsed.data;
                        currentCqData.columns = parsed.columns;
                    } else {
                        const buffer = e.target.result;
                        const base64 = arrayBufferToBase64(buffer);

                        console.log('Calling bridge.loadCqExcelFromBase64...');
                        const result = await bridge.loadCqExcelFromBase64(base64, 0, true);

                        const parsed = JSON.parse(result);
                        if (parsed && parsed.error) {
                            showNotification(parsed.error, 'danger');
                            return;
                        }

                        if (!Array.isArray(parsed.data)) {
                            console.error('parsed.data is not an array! Full parsed object:', JSON.stringify(parsed, null, 2));
                            showNotification('Invalid data format: data is not an array', 'danger');
                            return;
                        }

                        currentCqData = parsed.data;
                        currentCqData.columns = parsed.columns;
                    }

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

            if (isCsv) {
                reader.readAsText(file);
            } else {
                reader.readAsArrayBuffer(file);
            }
        } else {
            // Demo mode: load in browser
            if (isExcel) {
                showNotification('Excel import is only supported inside the desktop app.', 'warning');
                return;
            }
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
        const fileNameLower = file && file.name ? file.name.toLowerCase() : '';
        const isCsv = fileNameLower.endsWith('.csv');
        const isExcel = fileNameLower.endsWith('.xlsx') || fileNameLower.endsWith('.xls');
        if (!isCsv && !isExcel) {
            const lang = i18n.getLanguage ? i18n.getLanguage() : 'zh';
            const msgZh = '仅支持 CSV 或 Excel（.xlsx/.xls）导入。';
            const msgEn = 'Only CSV or Excel (.xlsx/.xls) import is supported here.';
            showNotification(lang === 'zh' ? msgZh : msgEn, 'warning');
            return;
        }
        console.log('=== Loading Design file ===');
        console.log('File name:', file.name);
        console.log('File size:', file.size);
        currentDesignFileName = file.name;
        console.log('File type:', file.type);

        if (bridge) {
            // Read file content and send to C++
            const reader = new FileReader();
            reader.onload = async function(e) {
                try {
                    if (isCsv) {
                        const content = e.target.result;
                        console.log('File content length:', content.length);
                        console.log('First 200 chars:', content.substring(0, 200));

                        console.log('Calling bridge.loadDesignFromContent...');
                        const result = await bridge.loadDesignFromContent(content);

                        const parsed = JSON.parse(result);
                        if (parsed && parsed.error) {
                            showNotification(parsed.error, 'danger');
                            return;
                        }

                        if (!Array.isArray(parsed.data)) {
                            console.error('parsed.data is not an array! Full parsed object:', JSON.stringify(parsed, null, 2));
                            showNotification('Invalid data format: data is not an array', 'danger');
                            return;
                        }

                        currentDesignData = parsed.data;
                        currentDesignData.columns = parsed.columns;
                    } else {
                        const buffer = e.target.result;
                        const base64 = arrayBufferToBase64(buffer);

                        console.log('Calling bridge.loadDesignExcelFromBase64...');
                        const result = await bridge.loadDesignExcelFromBase64(base64, 0, true);

                        const parsed = JSON.parse(result);
                        if (parsed && parsed.error) {
                            showNotification(parsed.error, 'danger');
                            return;
                        }

                        if (!Array.isArray(parsed.data)) {
                            console.error('parsed.data is not an array! Full parsed object:', JSON.stringify(parsed, null, 2));
                            showNotification('Invalid data format: data is not an array', 'danger');
                            return;
                        }

                        currentDesignData = parsed.data;
                        currentDesignData.columns = parsed.columns;
                    }

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

            if (isCsv) {
                reader.readAsText(file);
            } else {
                reader.readAsArrayBuffer(file);
            }
        } else {
            // Demo mode: load in browser
            if (isExcel) {
                showNotification('Excel import is only supported inside the desktop app.', 'warning');
                return;
            }
            loadDesignFile(file);
        }
    });

    // Design example data loading
    document.getElementById('loadDesignExampleBtn').addEventListener('click', function() {
        loadExampleDesignData();
    });

    // Concentration file loading (only if elements exist)
    const loadConcenBtn = document.getElementById('loadConcenBtn');
    if (loadConcenBtn) {
        loadConcenBtn.addEventListener('click', async function() {
        const fileInput = document.getElementById('concenFileInput');
        if (fileInput.files.length === 0) {
            showNotification(i18n.t('msg.noFileSelected'), 'warning');
            return;
        }

        const file = fileInput.files[0];
        const fileNameLower = file && file.name ? file.name.toLowerCase() : '';
        const isCsv = fileNameLower.endsWith('.csv');
        const isExcel = fileNameLower.endsWith('.xlsx') || fileNameLower.endsWith('.xls');
        if (!isCsv && !isExcel) {
            const lang = i18n.getLanguage ? i18n.getLanguage() : 'zh';
            const msgZh = '仅支持 CSV 或 Excel（.xlsx/.xls）导入。';
            const msgEn = 'Only CSV or Excel (.xlsx/.xls) import is supported here.';
            showNotification(lang === 'zh' ? msgZh : msgEn, 'warning');
            return;
        }
        console.log('=== Loading Concentration file ===');
        console.log('File name:', file.name);
        console.log('File size:', file.size);
        console.log('File type:', file.type);
        currentConcenFileName = file.name;

        if (bridge) {
            // Read file content and send to C++
            const reader = new FileReader();
            reader.onload = async function(e) {
                try {
                    if (isCsv) {
                        const content = e.target.result;
                        console.log('File content length:', content.length);
                        console.log('First 200 chars:', content.substring(0, 200));

                        console.log('Calling bridge.loadConcenFromContent...');
                        const result = await bridge.loadConcenFromContent(content);

                        const parsed = JSON.parse(result);
                        if (parsed && parsed.error) {
                            showNotification(parsed.error, 'danger');
                            return;
                        }

                        if (!Array.isArray(parsed.data)) {
                            console.error('parsed.data is not an array! Full parsed object:', JSON.stringify(parsed, null, 2));
                            showNotification('Invalid data format: data is not an array', 'danger');
                            return;
                        }

                        currentConcenData = parsed.data;
                        currentConcenData.columns = parsed.columns;
                    } else {
                        const buffer = e.target.result;
                        const base64 = arrayBufferToBase64(buffer);

                        console.log('Calling bridge.loadConcenExcelFromBase64...');
                        const result = await bridge.loadConcenExcelFromBase64(base64, 0, true);

                        const parsed = JSON.parse(result);
                        if (parsed && parsed.error) {
                            showNotification(parsed.error, 'danger');
                            return;
                        }

                        if (!Array.isArray(parsed.data)) {
                            console.error('parsed.data is not an array! Full parsed object:', JSON.stringify(parsed, null, 2));
                            showNotification('Invalid data format: data is not an array', 'danger');
                            return;
                        }

                        currentConcenData = parsed.data;
                        currentConcenData.columns = parsed.columns;
                    }

                    console.log('Final currentConcenData type:', Array.isArray(currentConcenData) ? 'array' : typeof currentConcenData);
                    console.log('Final currentConcenData.columns:', currentConcenData.columns);

                    displayConcenPreview(currentConcenData);
                    checkDataLoaded();
                    showNotification(i18n.t('msg.dataLoaded'), 'success');
                } catch (error) {
                    console.error('Error loading Concentration file:', error);
                    console.error('Error stack:', error.stack);
                    showNotification('Failed to parse file: ' + error.message, 'danger');
                }
            };
            reader.onerror = function(e) {
                console.error('FileReader error:', e);
                showNotification('Failed to read file', 'danger');
            };

            if (isCsv) {
                reader.readAsText(file);
            } else {
                reader.readAsArrayBuffer(file);
            }
        } else {
            // Demo mode: load in browser
            if (isExcel) {
                showNotification('Excel import is only supported inside the desktop app.', 'warning');
                return;
            }
            loadConcenFile(file);
        }
        });
    }

    // Concentration example data loading (only if element exists)
    const loadConcenExampleBtn = document.getElementById('loadConcenExampleBtn');
    if (loadConcenExampleBtn) {
        loadConcenExampleBtn.addEventListener('click', function() {
            loadExampleConcenData();
        });
    }

    // Proceed to analysis button
    document.getElementById('proceedToAnalysis').addEventListener('click', function() {
        if (currentCqData && (currentDesignData || currentConcenData)) {
            navigateToPage('analysis');
        }
    });
}

/**
 * Generic data file loader — replaces loadCqFile/loadDesignFile/loadConcenFile
 * @param {File} file - File object from input
 * @param {string} dialogTitle - i18n key for the file dialog title
 * @param {string} bridgeMethod - name of the bridge method (e.g. 'loadCqFile')
 * @param {string} previewFn - name of the preview function (e.g. 'displayCqPreview')
 * @param {string} dataSlot - name of the global variable to store data (e.g. 'currentCqData')
 */
function loadDataFile(file, dialogTitle, bridgeMethod, previewFn, dataSlot) {
    if (bridge) {
        bridge.showFileDialog(dialogTitle, '*.csv;;*.xlsx *.xls').then(filePath => {
            if (filePath) {
                try {
                    const result = bridge[bridgeMethod](filePath);
                    const parsed = JSON.parse(result);

                    if (!Array.isArray(parsed.data)) {
                        console.error('Invalid data format: data is not an array', parsed);
                        showNotification('Invalid data format: data is not an array', 'danger');
                        return;
                    }

                    window[dataSlot] = parsed.data;
                    window[dataSlot].columns = parsed.columns;
                    window[previewFn](window[dataSlot]);
                    checkDataLoaded();
                } catch (error) {
                    console.error('Error loading file via ' + bridgeMethod + ':', error);
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
                window[dataSlot] = data;
                window[previewFn](data);
                checkDataLoaded();
                showNotification(i18n.t('msg.dataLoaded'), 'success');
            } catch (error) {
                showNotification(i18n.t('msg.error') + ': ' + error.message, 'danger');
            }
        };
        reader.readAsText(file);
    }
}

function loadCqFile(file)     { loadDataFile(file, i18n.t('import.cqData'),     'loadCqFile',     'displayCqPreview',     'currentCqData'); }
function loadDesignFile(file) { loadDataFile(file, i18n.t('import.designData'), 'loadDesignFile', 'displayDesignPreview', 'currentDesignData'); }
function loadConcenFile(file) { loadDataFile(file, i18n.t('import.concenData'), 'loadConcenFile', 'displayConcenPreview', 'currentConcenData'); }

/**
 * Generic table preview renderer — replaces displayCqPreview/displayDesignPreview/displayConcenPreview
 * @param {string} tableId - DOM id of the <table> element
 * @param {Array} data - array of row objects, optionally with a .columns property
 * @param {string} [logLabel] - label for console logging
 */
function renderTablePreview(tableId, data, logLabel) {
    console.log('=== renderTablePreview ===', logLabel || tableId);

    const table = document.getElementById(tableId);
    if (!table) return;
    const thead = table.querySelector('thead tr');
    const tbody = table.querySelector('tbody');

    if (!data || !Array.isArray(data) || data.length === 0) {
        tbody.innerHTML = '<tr><td class="text-muted">' + i18n.t('table.noData') + '</td></tr>';
        return;
    }

    thead.innerHTML = '';
    tbody.innerHTML = '';

    const headers = data.columns || Object.keys(data[0]);
    headers.forEach(header => {
        const th = document.createElement('th');
        th.textContent = header;
        thead.appendChild(th);
    });

    data.slice(0, 10).forEach(row => {
        const tr = document.createElement('tr');
        headers.forEach(header => {
            const td = document.createElement('td');
            td.textContent = row[header];
            tr.appendChild(td);
        });
        tbody.appendChild(tr);
    });
}

// Convenience wrappers (kept for backwards compatibility)
function displayCqPreview(data)    { renderTablePreview('cqPreviewTable',      data, 'Cq'); }
function displayDesignPreview(data) { renderTablePreview('designPreviewTable',  data, 'Design'); }
function displayConcenPreview(data) { renderTablePreview('concenPreviewTable',  data, 'Concen'); }

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
        // Update reference gene datalist from Cq data
        const referenceGeneInput = document.getElementById('referenceGene');
        const geneList = document.getElementById('geneList');
        if (referenceGeneInput && geneList && currentCqData && Array.isArray(currentCqData)) {
            // Extract unique genes
            const genes = new Set();
            currentCqData.forEach(row => {
                if (row.Gene) {
                    genes.add(row.Gene);
                }
            });

            // Save current selection
            const currentValue = referenceGeneInput.value;

            // Clear existing options
            geneList.innerHTML = '';

            // Add new options to datalist
            genes.forEach(gene => {
                const option = document.createElement('option');
                option.value = gene;
                geneList.appendChild(option);
            });

            // Restore selection
            referenceGeneInput.value = currentValue;

            console.log('Updated reference gene datalist with', genes.size, 'unique genes');
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
            controlGroupSelect.innerHTML = '<option value="">' + i18n.t('param.selectControlGroup') + '</option>';

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
    const standardCurveParams = document.getElementById('standardCurveParams');
    const deltaCtParams = document.getElementById('deltaCtParams');
    const controlGroupField = document.getElementById('controlGroupField');
    const removeOutliersField = document.getElementById('removeOutliersField');
    const pAdjustField = document.getElementById('pAdjustField');
    const referenceGeneHelp = document.getElementById('referenceGeneHelp');
    const standardCurveMethodAlert = document.getElementById('standardCurveMethodAlert');

    if (method === 'standardCurve') {
        // Standard Curve method: hide concentration parameters, show basic params
        standardCurveParams.style.display = 'none';
        deltaCtParams.style.display = 'block';
        controlGroupField.style.display = 'block';
        removeOutliersField.style.display = 'none';
        pAdjustField.style.display = 'none';
        if (standardCurveMethodAlert) {
            standardCurveMethodAlert.style.display = 'block';
        }
        if (referenceGeneHelp) {
            referenceGeneHelp.textContent = 'Select a reference gene for normalization (e.g., GAPDH, Actin)';
        }
    } else if (method === 'deltaCt') {
        // Hide control group and standard curve params for ΔCt method
        standardCurveParams.style.display = 'none';
        deltaCtParams.style.display = 'block';
        controlGroupField.style.display = 'none';
        removeOutliersField.style.display = 'block';
        pAdjustField.style.display = 'block';
        if (standardCurveMethodAlert) {
            standardCurveMethodAlert.style.display = 'none';
        }
        if (referenceGeneHelp) {
            referenceGeneHelp.textContent = 'Select a reference gene for normalization (e.g., GAPDH, Actin)';
        }
    } else if (method === 'deltaDeltaCt') {
        // Show both for ΔΔCt method
        standardCurveParams.style.display = 'none';
        deltaCtParams.style.display = 'block';
        controlGroupField.style.display = 'block';
        removeOutliersField.style.display = 'block';
        pAdjustField.style.display = 'block';
        if (standardCurveMethodAlert) {
            standardCurveMethodAlert.style.display = 'none';
        }
        if (referenceGeneHelp) {
            referenceGeneHelp.textContent = 'Select a reference gene for normalization (e.g., GAPDH, Actin)';
        }
    } else if (method === 'standardCurveExp') {
        // Show both for Standard Curve Expression method
        standardCurveParams.style.display = 'none';
        deltaCtParams.style.display = 'block';
        controlGroupField.style.display = 'block';
        removeOutliersField.style.display = 'block';
        pAdjustField.style.display = 'block';
        if (standardCurveMethodAlert) {
            standardCurveMethodAlert.style.display = 'none';
        }
        if (referenceGeneHelp) {
            referenceGeneHelp.textContent = 'Use 2 reference genes (comma-separated) or leave blank for auto-selection by geNorm algorithm';
        }
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
    const pAdjustMethod = document.getElementById('pAdjustMethod').value;
    const removeOutliers = document.getElementById('removeOutliers').checked;
    const colorPalette = document.getElementById('colorPalette').value;

    // Standard Curve specific parameters
    const lowestConcen = parseFloat(document.getElementById('lowestConcen').value);
    const highestConcen = parseFloat(document.getElementById('highestConcen').value);
    const dilution = parseFloat(document.getElementById('dilution').value);
    const byMean = document.getElementById('byMean').checked;

    console.log('=== Analysis Parameters ===');
    console.log('Method:', method);
    console.log('Reference Gene:', referenceGene);
    console.log('Control Group:', controlGroup);
    console.log('Statistical Test:', statisticalTest);
    console.log('P-value adjustment:', pAdjustMethod);
    console.log('Remove Outliers:', removeOutliers);
    console.log('Color Palette:', colorPalette);
    console.log('Lowest Conc:', lowestConcen);
    console.log('Highest Conc:', highestConcen);
    console.log('Dilution:', dilution);
    console.log('By Mean:', byMean);

    // Validation
    if (method === 'standardCurve') {
        if (!currentConcenData || currentConcenData.length === 0) {
            showNotification('No concentration data loaded', 'danger');
            return;
        }
        if (isNaN(lowestConcen) || isNaN(highestConcen) || isNaN(dilution)) {
            showNotification('Invalid standard curve parameters', 'danger');
            return;
        }
        if (lowestConcen >= highestConcen) {
            showNotification('Lowest concentration must be less than highest concentration', 'danger');
            return;
        }
    } else {
        // All expression calculation methods require reference gene
        if (!referenceGene) {
            showNotification(i18n.t('msg.missingParams') + ': ' + i18n.t('param.referenceGene'), 'warning');
            return;
        }

        // deltaDeltaCt and standardCurveExp require control group
        if ((method === 'deltaDeltaCt' || method === 'standardCurveExp') && !controlGroup) {
            showNotification(i18n.t('msg.missingParams') + ': ' + i18n.t('param.controlGroup'), 'warning');
            return;
        }
    }

    // Check if data is loaded
    if (!currentCqData || currentCqData.length === 0) {
        showNotification('No Cq data loaded', 'danger');
        return;
    }

    if (method !== 'standardCurve' && (!currentDesignData || currentDesignData.length === 0)) {
        showNotification('No design data loaded', 'danger');
        return;
    }

    const params = {
        referenceGene: referenceGene,
        controlGroup: controlGroup,
        removeOutliers: removeOutliers,
        colorPalette: colorPalette,
        lowestConcen: lowestConcen,
        highestConcen: highestConcen,
        dilution: dilution,
        byMean: byMean
    };

    if (bridge) {
        try {
            // Send data to C++ backend first
            const cqJson = JSON.stringify(currentCqData);
            await bridge.setCqData(cqJson);

            // Send concentration data if using standard curve
            if (method === 'standardCurve') {
                console.log('Concentration data type:', Array.isArray(currentConcenData) ? 'array' : typeof currentConcenData);
                console.log('Concentration data length:', currentConcenData ? currentConcenData.length : 0);

                const concenJson = JSON.stringify(currentConcenData);
                console.log('Concentration JSON length:', concenJson.length);

                await bridge.setConcenData(concenJson);
            } else {
                // Send design data for deltaCt methods
                console.log('Design data type:', Array.isArray(currentDesignData) ? 'array' : typeof currentDesignData);
                console.log('Design data length:', currentDesignData ? currentDesignData.length : 0);

                const designJson = JSON.stringify(currentDesignData);
                console.log('Design JSON length:', designJson.length);

                await bridge.setDesignData(designJson);
            }

            console.log('Data sent to C++ successfully');

            // Call C++ backend based on selected method
            let result;
            if (method === 'standardCurve') {
                result = await bridge.calculateStandardCurve(JSON.stringify(params));
            } else if (method === 'deltaCt') {
                result = await bridge.calculateByDeltaCt(
                    JSON.stringify(params), statisticalTest, pAdjustMethod);
            } else if (method === 'deltaDeltaCt') {
                result = await bridge.calculateByDeltaDeltaCt(
                    JSON.stringify(params), statisticalTest, pAdjustMethod);
            } else if (method === 'standardCurveExp') {
                result = await bridge.calculateByStandardCurve(
                    JSON.stringify(params), statisticalTest, pAdjustMethod);
            }

            analysisResults = JSON.parse(result);

            runQcChecks();
            // 冻结本次分析的真实参数，供导出"分析记录"（导师 2026-10-14：分析过程要可核查）
            captureAnalysisParameters({
                method: method,
                referenceGene: referenceGene,
                controlGroup: controlGroup,
                statisticalTest: statisticalTest,
                pAdjustMethod: pAdjustMethod,
                removeOutliers: removeOutliers,
                lowestConcen: lowestConcen,
                highestConcen: highestConcen,
                dilution: dilution,
                byMean: byMean
            });

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
        runQcChecks();
        captureAnalysisParameters({
            method: method,
            referenceGene: referenceGene,
            controlGroup: controlGroup,
            statisticalTest: statisticalTest,
            pAdjustMethod: pAdjustMethod,
            removeOutliers: removeOutliers,
            lowestConcen: lowestConcen,
            highestConcen: highestConcen,
            dilution: dilution,
            byMean: byMean
        });

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
    // Check if this is a standard curve result
    if (results.method === 'standardCurve' || (results.table && results.table[0] && results.table[0].slope !== undefined)) {
        displayStandardCurveResults(results);
    } else {
        displayResultsTable(results);
        displayRawData(results);
        displayCharts(results);   // 2026-10-15 重新启用：由真实结果表绘制（bar + SD 误差线 + 显著性标注）
        // Statistics are now integrated into the main table
    }
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

    // 期望的列顺序；标曲表达量法的结果表带 SE 列，动态补上
    const headers = ['Gene', 'Group', 'Mean', 'StdDev'];
    if (results.table.some(row => row.SE !== undefined && row.SE !== null)) {
        headers.push('SE');
    }
    headers.push('PValue');
    if (results.table.some(row => row.RawPValue !== undefined && row.RawPValue !== null)) {
        headers.push('RawPValue', 'PAdjustMethod');
    }
    headers.push('Significance');
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
/**
 * 从分析结果构建 ECharts 配置（纯函数，便于 jsdom 测试）
 *
 * 兼容两种结果表：
 *   ① 汇总表：每行一个 (Gene, Group)，带 Mean / StdDev / Significance —— C++ 主流程输出；
 *   ② 逐重复表：带 Expression（演示模式或原始表），先按 (Gene, Group) 求 mean/SD。
 *
 * 图中每个基因一组柱（每组一色），柱顶误差线 = SD，柱顶标注显著性字母/星号。
 *
 * @param {Object} results 分析结果对象（需含 table 数组）
 * @param {Array<string>} palette 颜色数组（取自界面上选定的配色方案）
 * @returns {Object|null} ECharts option；无可用数据时返回 null
 */
function buildExpressionChartOption(results, palette) {
    if (!results || !Array.isArray(results.table) || results.table.length === 0) {
        return null;
    }

    const rows = results.table;
    const hasSummary = rows[0].Mean !== undefined;
    const genes = [];
    const groups = [];
    const cells = {};

    rows.forEach(function (row) {
        const gene = row.Gene === undefined ? '' : String(row.Gene);
        const group = row.Group === undefined ? '' : String(row.Group);
        if (!gene || !group) {
            return;
        }
        if (genes.indexOf(gene) < 0) {
            genes.push(gene);
        }
        if (groups.indexOf(group) < 0) {
            groups.push(group);
        }
        const key = gene + '\u001f' + group;
        if (hasSummary) {
            const mean = parseFloat(row.Mean);
            const sd = row.StdDev === undefined ? 0 : parseFloat(row.StdDev);
            if (!isNaN(mean)) {
                cells[key] = { mean: mean, sd: isNaN(sd) ? 0 : sd, sig: row.Significance || '', values: null };
            }
        } else {
            const value = parseFloat(row.Expression);
            if (!isNaN(value)) {
                if (!cells[key]) {
                    cells[key] = { mean: 0, sd: 0, sig: row.Significance || '', values: [] };
                }
                cells[key].values.push(value);
            }
        }
    });

    if (genes.length === 0 || groups.length === 0) {
        return null;
    }

    if (!hasSummary) {
        Object.keys(cells).forEach(function (key) {
            const values = cells[key].values;
            const mean = values.reduce(function (a, b) { return a + b; }, 0) / values.length;
            const variance = values.length > 1
                ? values.reduce(function (a, b) { return a + (b - mean) * (b - mean); }, 0) / (values.length - 1)
                : 0;
            cells[key].mean = mean;
            cells[key].sd = Math.sqrt(variance);
        });
    }

    const colors = Array.isArray(palette) && palette.length > 0 ? palette : ['#3C5488'];
    const series = [];
    const errorBars = [];
    const rawValues = {};
    if (Array.isArray(results.rawData)) {
        results.rawData.forEach(function (row) {
            const gene = row.Gene === undefined ? '' : String(row.Gene);
            const group = row.Group === undefined ? '' : String(row.Group);
            const value = parseFloat(row.Expression);
            if (!gene || !group || isNaN(value)) {
                return;
            }
            const key = gene + '\u001f' + group;
            if (!rawValues[key]) {
                rawValues[key] = [];
            }
            rawValues[key].push(value);
        });
    }

    // 柱宽固定成 band 的固定百分比，柱、误差线、散点和显著性标记才有确定的水平偏移。
    // ECharts 里 barWidth:'P%' 的百分比基准 = 类目带宽，而自定义 series 里
    // 能拿到同一个带宽的通道就是 api.size([1, 0])[0]（2026-10-15 用 5.4.4 的
    // SSR 渲染实测：band=905.2、barWidth '22%' → 柱宽 199.1 = 905.2×0.22）。
    // 不显式指定时由 ECharts 自行分配 barGap/barCategoryGap，算出来的偏移
    // 与真实柱心对不上——曾出现"第 1、3 根柱子的误差线明显偏右"。
    const barGapPct = 12;
    const barWidthPct = 100 / groups.length * 0.64;
    const barStepFactor = 1 + barGapPct / 100;
    const barWidthFraction = barWidthPct / 100;
    function barMetrics(api) {
        const barWidth = api.size([1, 0])[0] * barWidthFraction;
        return {
            barWidth: barWidth,
            barStep: barWidth * barStepFactor
        };
    }

    groups.forEach(function (group, groupIndex) {
        const data = genes.map(function (gene) {
            const cell = cells[gene + '\u001f' + group];
            return cell ? Number(cell.mean.toPrecision(12)) : null;
        });

        genes.forEach(function (gene, geneIndex) {
            const cell = cells[gene + '\u001f' + group];
            if (!cell) {
                return;
            }
            // [类目序号, 下限, 水平偏移(渲染时填), 上限]
            errorBars.push([geneIndex, cell.mean - cell.sd, groupIndex, cell.mean + cell.sd]);
        });

        series.push({
            name: group,
            type: 'bar',
            barGap: barGapPct + '%',
            barWidth: barWidthPct + '%',
            data: data,
            itemStyle: { color: colors[groupIndex % colors.length] }
        });

        const pointData = [];
        const labelData = [];
        genes.forEach(function (gene, geneIndex) {
            const cell = cells[gene + '\u001f' + group];
            if (!cell) {
                return;
            }
            const observations = rawValues[gene + '\u001f' + group] || [];
            if (observations.length > 0) {
                observations.sort(function (a, b) { return a - b; });
                const spread = 0.48;
                observations.forEach(function (value, observationIndex) {
                    const jitterFraction = observations.length > 1
                        ? (observationIndex / (observations.length - 1) - 0.5) * spread
                        : 0;
                    pointData.push([geneIndex, value, jitterFraction]);
                });
            }
            if (cell.sig) {
                labelData.push([geneIndex, cell.mean + cell.sd, cell.sig]);
            }
        });

        if (pointData.length > 0) {
            series.push({
                name: group + ' observations',
                type: 'custom',
                z: 12,
                renderItem: function (params, api) {
                    const geneIndex = api.value(0);
                    const value = api.value(1);
                    const jitterFraction = api.value(2);
                    const metrics = barMetrics(api);
                    const groupOffset = (groupIndex - (groups.length - 1) / 2)
                        * metrics.barStep;
                    const x = api.coord([geneIndex, 0])[0] + groupOffset
                        + jitterFraction * metrics.barWidth;
                    const y = api.coord([geneIndex, value])[1];
                    return {
                        type: 'circle',
                        shape: { cx: x, cy: y, r: 3.5 },
                        style: {
                            fill: colors[groupIndex % colors.length],
                            stroke: '#222222',
                            lineWidth: 0.7,
                            opacity: 0.82
                        }
                    };
                },
                data: pointData
            });
        }
        if (labelData.length > 0) {
            series.push({
                name: group + ' significance',
                type: 'custom',
                z: 14,
                renderItem: function (params, api) {
                    const geneIndex = api.value(0);
                    const yValue = api.value(1);
                    const sig = api.value(2);
                    const metrics = barMetrics(api);
                    const groupOffset = (groupIndex - (groups.length - 1) / 2)
                        * metrics.barStep;
                    const x = api.coord([geneIndex, 0])[0] + groupOffset;
                    const y = api.coord([geneIndex, yValue])[1] - 7;
                    return {
                        type: 'text',
                        style: {
                            text: sig,
                            x: x,
                            y: y,
                            fill: '#111111',
                            font: 'bold 14px sans-serif',
                            align: 'center',
                            verticalAlign: 'bottom'
                        }
                    };
                },
                data: labelData
            });
        }
    });

    // SD 误差线（custom series，逐项渲染：每项返回竖线 + 上下两条端帽）。
    //
    // 两个坑（2026-10-15 用 ECharts 5.4.4 的 SSR 渲染实测确认）：
    //   ① renderItem 的 params.data 在该版本里是 undefined，取当前项必须用
    //      api.value(dim)；用 params.data.forEach(...) 会直接抛 TypeError，
    //      异常被渲染循环吞掉，界面上**一根误差线都画不出来**（图注与结果一节
    //      却都写了 SD 误差线）。
    //   ② 早先照"官方 error-bar 示例"用 params.context.rendered 在首项一次性渲染
    //      整张表的写法同样不成立：ECharts 是按每个数据项调用 renderItem 的。
    series.push({
        name: 'SD',
        type: 'custom',
        silent: true,
        z: 10,
        renderItem: function (params, api) {
            // 当前项的 4 个维度：[geneIndex, lower, groupIndex, upper]
            const geneIndex = api.value(0);
            const lower = api.value(1);
            const groupIndex = api.value(2);
            const upper = api.value(3);
            // 与 bar 系列的 barWidth:'<barWidthPct>%' 用同一个基准（类目带宽），
            // 保证误差线落在各自柱子的正中间。
            const metrics = barMetrics(api);
            const offset = (groupIndex - (groups.length - 1) / 2) * metrics.barStep;
            const x = api.coord([geneIndex, 0])[0] + offset;
            const capHalf = Math.max(3, metrics.barWidth * 0.18);
            const yLow = api.coord([geneIndex, lower])[1];
            const yHigh = api.coord([geneIndex, upper])[1];
            return {
                type: 'group',
                children: [{
                    type: 'line',
                    shape: { x1: x, y1: yLow, x2: x, y2: yHigh },
                    style: { stroke: '#555555', lineWidth: 1 }
                }, {
                    type: 'line',
                    shape: { x1: x - capHalf, y1: yLow, x2: x + capHalf, y2: yLow },
                    style: { stroke: '#555555', lineWidth: 1 }
                }, {
                    type: 'line',
                    shape: { x1: x - capHalf, y1: yHigh, x2: x + capHalf, y2: yHigh },
                    style: { stroke: '#555555', lineWidth: 1 }
                }]
            };
        },
        data: errorBars
    });

    return {
        title: {
            text: 'Relative expression',
            left: 'center',
            textStyle: { fontSize: 15, fontWeight: 'bold' }
        },
        tooltip: { trigger: 'axis', axisPointer: { type: 'shadow' } },
        legend: { top: 26, left: 'center', data: groups },
        grid: { left: '3%', right: '4%', bottom: '12%', top: '18%', containLabel: true },
        xAxis: {
            type: 'category',
            data: genes,
            axisLabel: { rotate: genes.length > 4 ? 45 : 0, fontSize: 12 }
        },
        yAxis: {
            type: 'value',
            name: 'Relative expression',
            axisLabel: { fontSize: 12 },
            max: function (value) {
                return Math.ceil(value.max * 1.16 * 10) / 10;
            }
        },
        series: series
    };
}

/**
 * 渲染表达量柱状图（真实数据；图在结果页的 #mainChart 容器内）
 *
 * @param {Object} results 分析结果对象
 * @returns {boolean} 是否成功渲染
 */
function displayCharts(results) {
    const chartDom = document.getElementById('mainChart');
    if (!chartDom || typeof echarts === 'undefined') {
        return false;
    }

    const paletteSelect = document.getElementById('colorPalette');
    const palette = colorPalettes[paletteSelect ? paletteSelect.value : 'nature'] || colorPalettes.nature;
    const option = buildExpressionChartOption(results, palette);
    if (!option) {
        return false;
    }

    if (mainChart) {
        mainChart.dispose();
    }
    mainChart = echarts.init(chartDom);
    mainChart.setOption(option);

    // 只在第一次挂 resize 监听，避免每次分析都叠加一个监听器
    if (!displayCharts.resizeBound) {
        displayCharts.resizeBound = true;
        window.addEventListener('resize', function () {
            if (mainChart) {
                mainChart.resize();
            }
        });
    }
    return true;
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

    // 分析记录导出（输入、参数、输出规模一览；导师 2026-10-14 意见：让分析过程可核查、可重跑）
    const recordBtn = document.getElementById('exportAnalysisRecord');
    if (recordBtn) {
        recordBtn.addEventListener('click', function() {
            exportAnalysisRecord();
        });
    }

    // 图表导出（2026-10-15 随作图功能一起恢复）
    const chartExportBtn = document.getElementById('exportChart');
    if (chartExportBtn) {
        chartExportBtn.addEventListener('click', function() {
            exportChart();
        });
    }
}

/**
 * 质控预警的默认阈值
 *
 * 说明（避免误导）：MIQE 要求**报告**效率、技术重复离散度、检测限附近的数据，
 * 但并没有规定统一的判废阈值；下面这些只是本软件给出的**预警**默认值，
 * 可以按仪器与实验体系调整，且软件只提示、从不自动删除数据。
 *  - cqUpper        35      ：高于此值接近常见检测上限，应结合 LOD/扩增曲线核查
 *  - techRepSd      0.5 Cq  ：同一生物学样本的技术重复标准差
 *  - efficiencyLow/High 0.90/1.10：效率 90–110%（对应斜率约 −3.1 到 −3.6）
 */
const QC_THRESHOLDS = {
    cqUpper: 35,
    techRepSd: 0.5,
    efficiencyLow: 0.90,
    efficiencyHigh: 1.10
};

let lastQcReport = null;

/**
 * 计算质控预警（纯函数，便于 jsdom 测试）
 *
 * @param {Object} input {cqData, designData, concenData, scResults, thresholds}
 * @returns {Object} {thresholds, items: [{code, count, message}]}
 */
function buildQcReport(input) {
    const thresholds = Object.assign({}, QC_THRESHOLDS, (input && input.thresholds) || {});
    const items = [];
    const cqData = input && Array.isArray(input.cqData) ? input.cqData : [];
    const designData = input && Array.isArray(input.designData) ? input.designData : [];
    const scTable = (input && input.scResults && Array.isArray(input.scResults.table))
        ? input.scResults.table : [];

    if (cqData.length === 0) {
        return { thresholds: thresholds, items: items };
    }

    // ① Cq 偏高（接近检测上限）
    const highCq = cqData.filter(function (row) {
        const cq = parseFloat(row.Cq);
        return !isNaN(cq) && cq > thresholds.cqUpper;
    });
    if (highCq.length > 0) {
        items.push({
            code: 'cqHigh',
            count: highCq.length,
            message: highCq.length + ' well(s) have Cq > ' + thresholds.cqUpper
                + '; check the amplification curves, negative controls and the limit of detection'
        });
    }

    // ② 技术重复离散度（按 Gene × Group × BioRep 分组，用孔级 Cq 求 SD）
    if (designData.length > 0) {
        const designByPosition = {};
        designData.forEach(function (row) {
            if (row.Position !== undefined) {
                designByPosition[String(row.Position)] = row;
            }
        });

        const cqBySample = {};
        cqData.forEach(function (row) {
            const position = row.Position === undefined ? '' : String(row.Position);
            const meta = designByPosition[position];
            if (!meta) {
                return;
            }
            const cq = parseFloat(row.Cq);
            if (isNaN(cq)) {
                return;
            }
            const key = [row.Gene === undefined ? '' : String(row.Gene),
                         meta.Group === undefined ? '' : String(meta.Group),
                         meta.BioRep === undefined ? '' : String(meta.BioRep)].join('|');
            if (!cqBySample[key]) {
                cqBySample[key] = [];
            }
            cqBySample[key].push(cq);
        });

        const noisy = [];
        Object.keys(cqBySample).forEach(function (key) {
            const values = cqBySample[key];
            if (values.length < 2) {
                return;
            }
            const mean = values.reduce(function (a, b) { return a + b; }, 0) / values.length;
            const variance = values.reduce(function (a, b) { return a + (b - mean) * (b - mean); }, 0)
                / (values.length - 1);
            const sd = Math.sqrt(variance);
            if (sd > thresholds.techRepSd) {
                noisy.push({ key: key, sd: sd });
            }
        });
        if (noisy.length > 0) {
            items.push({
                code: 'techRepSd',
                count: noisy.length,
                message: noisy.length + ' sample(s) have a technical-replicate SD > '
                    + thresholds.techRepSd + ' Cq (worst: ' + noisy[0].key + ', SD = '
                    + noisy[0].sd.toFixed(2) + ')'
            });
        }
    }

    // ③ 扩增效率超出常用区间（只对标准曲线结果）
    const lowEff = 100 * thresholds.efficiencyLow;
    const highEff = 100 * thresholds.efficiencyHigh;
    const badEfficiency = scTable.filter(function (row) {
        const efficiency = parseFloat(row.Efficiency);
        return !isNaN(efficiency) && (efficiency < thresholds.efficiencyLow || efficiency > thresholds.efficiencyHigh);
    });
    if (badEfficiency.length > 0) {
        items.push({
            code: 'efficiency',
            count: badEfficiency.length,
            message: badEfficiency.length + ' primer(s) outside ' + lowEff + '-' + highEff + '% efficiency: '
                + badEfficiency.map(function (row) { return row.Gene; }).join(', ')
        });
    }

    return { thresholds: thresholds, items: items };
}

/**
 * 用当前会话的数据跑质控预警，并把结果渲染到结果页（只提示，不改数据）
 *
 * @returns {Object} buildQcReport() 的结果
 */
function runQcChecks() {
    lastQcReport = buildQcReport({
        cqData: currentCqData || scCqData,
        designData: currentDesignData,
        concenData: currentConcenData || scConcenData,
        scResults: scResults
    });
    displayQcReport(lastQcReport);
    return lastQcReport;
}

/**
 * 渲染质控预警列表
 *
 * @param {Object} report buildQcReport() 的结果
 */
function displayQcReport(report) {
    const section = document.getElementById('qcSection');
    const list = document.getElementById('qcList');
    if (!section || !list) {
        return;
    }

    if (!report || report.items.length === 0) {
        section.style.display = 'none';
        list.innerHTML = '';
        return;
    }

    const heading = '<strong>' + i18n.t('qc.title') + '</strong>';
    const body = report.items.map(function (item) {
        return '<li>' + item.message + '</li>';
    }).join('');
    list.innerHTML = heading + '<ul class="mb-0 mt-1">' + body + '</ul>';
    section.style.display = 'block';
}

/**
 * 冻结本次分析的输入与参数到 lastAnalysisParameters（导出"分析记录"用）
 *
 * @param {Object} ctx 本次运行的界面参数（method/referenceGene/controlGroup/...）
 */
function captureAnalysisParameters(ctx) {
    const columnCount = function (data) {
        if (!data || data.length === 0) {
            return 0;
        }
        if (Array.isArray(data.columns)) {
            return data.columns.length;
        }
        return data[0] ? Object.keys(data[0]).length : 0;
    };

    lastAnalysisParameters = {
        method: ctx.method,
        methodLabel: i18n.t('method.' + ctx.method),
        referenceGene: ctx.referenceGene,
        controlGroup: ctx.controlGroup,
        statisticalTest: ctx.statisticalTest,
        pAdjustMethod: ctx.pAdjustMethod,
        removeOutliers: ctx.removeOutliers,
        lowestConcen: ctx.lowestConcen,
        highestConcen: ctx.highestConcen,
        dilution: ctx.dilution,
        byMean: ctx.byMean,
        cqFileName: currentCqFileName,
        cqRows: currentCqData ? currentCqData.length : 0,
        cqColumns: columnCount(currentCqData),
        designFileName: currentDesignFileName,
        designRows: currentDesignData ? currentDesignData.length : 0,
        designColumns: columnCount(currentDesignData),
        concenFileName: currentConcenFileName,
        concenRows: currentConcenData ? currentConcenData.length : 0,
        summaryRows: (analysisResults && analysisResults.table) ? analysisResults.table.length : 0,
        rawRows: (analysisResults && analysisResults.rawData) ? analysisResults.rawData.length : 0,
        statisticCount: (analysisResults && Array.isArray(analysisResults.statistics))
            ? analysisResults.statistics.length : 0,
        qcItems: (lastQcReport && Array.isArray(lastQcReport.items)) ? lastQcReport.items : []
    };
}

/**
 * 构建"分析记录"文本（纯函数，便于 jsdom 单元测试）
 *
 * 记录一次分析实际用到的输入、参数与输出规模，使分析过程可核查、可重跑。
 *
 * @param {Object} snapshot 最近一次成功分析的参数与输入信息快照
 * @param {string} appVersion 软件版本（来自 C++ 端 getAppVersion）
 * @param {string} timestamp 生成本记录的时间字符串
 * @returns {string} 记录文本；snapshot 为空时返回空串
 */
function buildAnalysisRecord(snapshot, appVersion, timestamp) {
    if (!snapshot) {
        return '';
    }

    const lines = [];
    const field = function (label, value) {
        const shown = (value === undefined || value === null || value === '') ? '-' : value;
        lines.push(label.padEnd(24, ' ') + ': ' + shown);
    };

    lines.push('qPCRtools analysis record');
    lines.push('=========================');
    lines.push('');
    lines.push('[Run]');
    field('Generated at', timestamp);
    field('Software version', appVersion);
    field('Method', snapshot.methodLabel || snapshot.method);
    lines.push('');
    lines.push('[Inputs]');
    field('Cq file', snapshot.cqFileName);
    field('Cq rows x columns', snapshot.cqRows + ' x ' + snapshot.cqColumns);
    if (snapshot.method === 'standardCurve') {
        field('Concentration file', snapshot.concenFileName);
        field('Concentration rows', snapshot.concenRows);
    } else {
        field('Design file', snapshot.designFileName);
        field('Design rows x columns', snapshot.designRows + ' x ' + snapshot.designColumns);
    }
    lines.push('');
    lines.push('[Parameters]');
    if (snapshot.method === 'standardCurve') {
        field('Lowest concentration', snapshot.lowestConcen);
        field('Highest concentration', snapshot.highestConcen);
        field('Dilution factor', snapshot.dilution);
        field('Average technical reps', snapshot.byMean ? 'yes' : 'no');
    } else {
        field('Reference gene(s)', snapshot.referenceGene);
        field('Control group', snapshot.controlGroup);
        field('Statistical test', snapshot.statisticalTest);
        field('P-value adjustment', snapshot.pAdjustMethod);
        field('Remove outliers (IQR)', snapshot.removeOutliers ? 'yes' : 'no');
    }
    lines.push('');
    lines.push('[Output]');
    field('Summary rows', snapshot.summaryRows);
    field('Raw rows', snapshot.rawRows);
    field('Statistical results', snapshot.statisticCount);
    lines.push('');
    const qcItems = Array.isArray(snapshot.qcItems) ? snapshot.qcItems : [];
    lines.push('[Quality control warnings]');
    if (qcItems.length === 0) {
        lines.push('- none (no well exceeded the default warning thresholds)');
    } else {
        qcItems.forEach(function (item) {
            lines.push('- ' + item.message);
        });
    }
    lines.push('  (warnings only; no data were removed)');
    lines.push('');
    lines.push('[Notes]');
    lines.push('- Group comparisons are performed on the linear relative-expression scale');
    lines.push('  (Welch t-test, Wilcoxon rank-sum, or one-way ANOVA with Tukey HSD),');
    lines.push('  as in the R implementation.');
    if (snapshot.statisticalTest === 'anova') {
        lines.push('- P values are the overall ANOVA P value; letters are Tukey HSD groupings.');
    } else {
        lines.push('- P values are adjusted within each gene across comparisons against the control');
        lines.push('  using the method listed above; raw P values are retained in exported tables.');
    }
    lines.push('- Re-running the same input files with the parameters listed above');
    lines.push('  reproduces this analysis.');
    lines.push('');
    return lines.join('\n');
}

/**
 * 导出"分析记录"（.txt）：把最近一次成功分析的输入与参数写盘
 */
async function exportAnalysisRecord() {
    if (!lastAnalysisParameters) {
        showNotification(i18n.t('msg.noData'), 'warning');
        return;
    }

    const now = new Date();
    const stamp = now.toISOString().slice(0, 19).replace('T', ' ');
    const datePart = now.toISOString().slice(0, 10).replace(/-/g, '');
    let version = 'unknown';
    if (bridge && bridge.getAppVersion) {
        try {
            version = await bridge.getAppVersion();
        } catch (e) {
            version = 'unknown';
        }
    }

    const text = buildAnalysisRecord(lastAnalysisParameters, version, stamp);
    if (!text) {
        showNotification(i18n.t('msg.noData'), 'warning');
        return;
    }

    const defaultName = 'analysis_record_' + datePart;
    if (bridge) {
        bridge.showSaveDialog(i18n.t('btn.analysisRecord'), '*.txt', defaultName).then(filePath => {
            if (filePath) {
                bridge.saveTextFile(text, filePath);
                showNotification(i18n.t('msg.analysisCompleted'), 'success');
            }
        });
    } else {
        downloadFile(text, defaultName + '.txt', 'text/plain');
        showNotification(i18n.t('msg.analysisCompleted'), 'success');
    }
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
    if (!mainChart || typeof mainChart.getDataURL !== 'function') {
        showNotification(i18n.t('msg.noData'), 'warning');
        return;
    }

    const url = mainChart.getDataURL({
        type: 'png',
        pixelRatio: 3,
        backgroundColor: '#fff'
    });

    const link = document.createElement('a');
    link.href = url;
    link.download = 'qpcrtools_figure.png';
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
    if (lines.length === 0) return [];

    // Detect delimiter: comma or tab
    const firstLine = lines[0];
    const delimiter = firstLine.includes('\t') ? '\t' : ',';

    const headers = firstLine.split(delimiter).map(h => h.trim());
    const data = [];

    for (let i = 1; i < lines.length; i++) {
        const line = lines[i].trim();
        if (!line) continue; // Skip empty lines

        const values = line.split(delimiter).map(v => v.trim());
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

/**
 * Convert ArrayBuffer (binary) to base64 string.
 * Used for sending Excel files to the native Qt backend.
 */
function arrayBufferToBase64(buffer) {
    const bytes = new Uint8Array(buffer);
    const chunkSize = 0x8000; // Avoid call stack overflow in String.fromCharCode/apply
    let binary = '';
    for (let i = 0; i < bytes.length; i += chunkSize) {
        const chunk = bytes.subarray(i, i + chunkSize);
        binary += String.fromCharCode.apply(null, chunk);
    }
    return btoa(binary);
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


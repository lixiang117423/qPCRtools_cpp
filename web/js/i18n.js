/**
 * qPCRtools Internationalization (i18n)
 * Supports Chinese and English
 */

const translations = {
    zh: {
        // App
        'app.title': 'qPCRtools',

        // Navigation
        'nav.home': '首页',
        'nav.import': '导入数据',
        'nav.analysis': '分析配置',
        'nav.results': '结果展示',
        'nav.templates': '模板下载',

        // Home
        'home.welcome': '欢迎使用 qPCRtools',
        'home.subtitle': '专业的qPCR数据分析软件',
        'home.description': '支持标准曲线分析、ΔCt/ΔΔCt方法、统计检验和ggplot2风格可视化。',

        // Templates Modal
        'templates.title': '下载模板',
        'templates.description': '下载模板和示例数据开始使用qPCRtools。',
        'templates.cqTemplate': 'Cq数据模板',
        'templates.cqTemplateDesc': 'Cq值的CSV模板',
        'templates.designTemplate': '实验设计模板',
        'templates.designTemplateDesc': '样本分组的CSV模板',
        'templates.exampleData': '示例数据包',
        'templates.exampleDataDesc': '完整示例文件（Cq + Design）',
        'templates.info': '模板文件在第一行包含说明。下载它们以了解所需的数据格式。',
        'templates.close': '关闭',

        // Buttons
        'btn.startAnalysis': '开始分析',
        'btn.loadExample': '加载示例数据',
        'btn.loadExampleData': '加载示例数据',
        'btn.load': '加载文件',
        'btn.proceedToAnalysis': '进入分析',
        'btn.runAnalysis': '运行分析',
        'btn.export': '导出',
        'btn.close': '关闭',
        'btn.download': '下载',

        // Features
        'feature.standardCurve': '标准曲线',
        'feature.standardCurveDesc': '线性回归和扩增效率计算',
        'feature.deltaCt': 'ΔCt/ΔΔCt',
        'feature.deltaCtDesc': '相对定量分析方法',
        'feature.statistics': '统计检验',
        'feature.statisticsDesc': 't检验、方差分析、秩和检验',
        'feature.visualization': '数据可视化',
        'feature.visualizationDesc': 'ggplot2风格的出版级图表',

        // Import
        'import.title': '导入数据',
        'import.cqData': 'Cq数据',
        'import.designData': '实验设计',
        'import.fileFormat': '文件格式',
        'import.selectFile': '选择文件',

        // Analysis
        'analysis.title': '配置分析参数',
        'analysis.method': '分析方法',
        'analysis.parameters': '参数设置',

        // Methods
        'method.standardCurve': '标准曲线法',
        'method.deltaCt': 'ΔCt法',
        'method.deltaDeltaCt': 'ΔΔCt法',

        // Parameters
        'param.referenceGene': '参考基因',
        'param.controlGroup': '对照组',
        'param.statisticalTest': '统计检验方法',
        'param.removeOutliers': '移除异常值（IQR方法）',
        'param.colorPalette': '配色方案',

        // Results
        'results.title': '分析结果',
        'results.data': '数据',
        'results.charts': '图表',
        'results.summaryData': '汇总数据',
        'results.plots': '图表展示',

        // Table
        'table.preview': '预览',
        'table.noData': '未加载数据',
        'table.gene': '基因',
        'table.comparison': '比较',
        'table.test': '检验',
        'table.statistic': '统计量',
        'table.pvalue': 'P值',
        'table.significance': '显著性',

        // Progress
        'progress.title': '处理中',

        // Messages
        'msg.dataLoaded': '数据加载成功',
        'msg.analysisCompleted': '分析完成',
        'msg.error': '错误',
        'msg.noFileSelected': '未选择文件',
        'msg.invalidFile': '无效的文件格式',
        'msg.missingParams': '缺少必要参数',
        'msg.processing': '正在处理...',

        // Statistical significance
        'sig.ns': 'NS',
        'sig.significant': '*',
        'sig.verySignificant': '**',
        'sig.extremelySignificant': '***'
    },

    en: {
        // App
        'app.title': 'qPCRtools',

        // Navigation
        'nav.home': 'Home',
        'nav.import': 'Import Data',
        'nav.analysis': 'Analysis',
        'nav.results': 'Results',
        'nav.templates': 'Templates',

        // Home
        'home.welcome': 'Welcome to qPCRtools',
        'home.subtitle': 'Professional qPCR Data Analysis Software',
        'home.description': 'Support for standard curve analysis, ΔCt/ΔΔCt methods, statistical tests, and ggplot2-style visualization.',

        // Templates Modal
        'templates.title': 'Download Templates',
        'templates.description': 'Download templates and example data to get started with qPCRtools.',
        'templates.cqTemplate': 'Cq Data Template',
        'templates.cqTemplateDesc': 'CSV template for Cq values',
        'templates.designTemplate': 'Experimental Design Template',
        'templates.designTemplateDesc': 'CSV template for sample groups',
        'templates.exampleData': 'Example Data Package',
        'templates.exampleDataDesc': 'Complete example files (Cq + Design)',
        'templates.info': 'Template files include instructions in the first row. Download them to understand the required data format.',
        'templates.close': 'Close',

        // Buttons
        'btn.startAnalysis': 'Start Analysis',
        'btn.loadExample': 'Load Example',
        'btn.loadExampleData': 'Load Example Data',
        'btn.load': 'Load File',
        'btn.proceedToAnalysis': 'Proceed to Analysis',
        'btn.runAnalysis': 'Run Analysis',
        'btn.export': 'Export',
        'btn.close': 'Close',
        'btn.download': 'Download',

        // Features
        'feature.standardCurve': 'Standard Curve',
        'feature.standardCurveDesc': 'Linear regression and amplification efficiency',
        'feature.deltaCt': 'ΔCt/ΔΔCt',
        'feature.deltaCtDesc': 'Relative quantification methods',
        'feature.statistics': 'Statistical Tests',
        'feature.statisticsDesc': 't-test, ANOVA, Wilcoxon test',
        'feature.visualization': 'Visualization',
        'feature.visualizationDesc': 'ggplot2-style publication quality charts',

        // Import
        'import.title': 'Import Data',
        'import.cqData': 'Cq Data',
        'import.designData': 'Experimental Design',
        'import.fileFormat': 'File Format',
        'import.selectFile': 'Select File',

        // Analysis
        'analysis.title': 'Configure Analysis',
        'analysis.method': 'Analysis Method',
        'analysis.parameters': 'Parameters',

        // Methods
        'method.standardCurve': 'Standard Curve',
        'method.deltaCt': 'ΔCt Method',
        'method.deltaDeltaCt': 'ΔΔCt Method',

        // Parameters
        'param.referenceGene': 'Reference Gene',
        'param.controlGroup': 'Control Group',
        'param.statisticalTest': 'Statistical Test',
        'param.removeOutliers': 'Remove Outliers (IQR method)',
        'param.colorPalette': 'Color Palette',

        // Results
        'results.title': 'Analysis Results',
        'results.data': 'Data',
        'results.charts': 'Charts',
        'results.summaryData': 'Summary Data',
        'results.plots': 'Plots',

        // Table
        'table.preview': 'Preview',
        'table.noData': 'No data loaded',
        'table.gene': 'Gene',
        'table.comparison': 'Comparison',
        'table.test': 'Test',
        'table.statistic': 'Statistic',
        'table.pvalue': 'P-value',
        'table.significance': 'Significance',

        // Progress
        'progress.title': 'Processing',

        // Messages
        'msg.dataLoaded': 'Data loaded successfully',
        'msg.analysisCompleted': 'Analysis completed',
        'msg.error': 'Error',
        'msg.noFileSelected': 'No file selected',
        'msg.invalidFile': 'Invalid file format',
        'msg.missingParams': 'Missing required parameters',
        'msg.processing': 'Processing...',

        // Statistical significance
        'sig.ns': 'NS',
        'sig.significant': '*',
        'sig.verySignificant': '**',
        'sig.extremelySignificant': '***'
    }
};

class I18n {
    constructor() {
        this.currentLanguage = 'zh';
        this.loadLanguage();
    }

    loadLanguage() {
        // Try to load from localStorage
        const savedLang = localStorage.getItem('language');
        if (savedLang && translations[savedLang]) {
            this.currentLanguage = savedLang;
        }
    }

    setLanguage(language) {
        if (translations[language]) {
            this.currentLanguage = language;
            localStorage.setItem('language', language);
            this.updateUIText();
            this.updateLanguageDisplay();
        }
    }

    getLanguage() {
        return this.currentLanguage;
    }

    t(key) {
        return translations[this.currentLanguage][key] || key;
    }

    updateUIText() {
        // Update all elements with data-i18n attribute
        document.querySelectorAll('[data-i18n]').forEach(element => {
            const key = element.getAttribute('data-i18n');
            const translation = this.t(key);
            element.textContent = translation;
        });

        // Update placeholders
        document.querySelectorAll('[data-i18n-placeholder]').forEach(element => {
            const key = element.getAttribute('data-i18n-placeholder');
            const translation = this.t(key);
            element.setAttribute('placeholder', translation);
        });
    }

    updateLanguageDisplay() {
        const langDisplay = this.currentLanguage === 'zh' ? '中文' : 'English';
        const langElement = document.getElementById('currentLanguage');
        if (langElement) {
            langElement.textContent = langDisplay;
        }
        document.documentElement.lang = this.currentLanguage === 'zh' ? 'zh-CN' : 'en';
    }
}

// Create global i18n instance
const i18n = new I18n();

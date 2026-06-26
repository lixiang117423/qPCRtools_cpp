// 模板与示例数据下载函数（从 app.js 抽取，行为不变）。
// 作为独立 <script> 加载，与 app.js 共享全局（downloadFile / i18n / showNotification 等）。

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

/**
 * Download design template with efficiency column
 */
function downloadDesignWithEffTemplate() {
    const csv = `Position,Group,Gene,BioRep,TechRep,Eff
A1,CK,RefGene1,1,1,1.95
A2,CK,RefGene2,1,1,2.00
A3,CK,TargetGene1,1,1,1.98
A4,Treatment,RefGene1,1,1,1.95
A5,Treatment,RefGene2,1,1,2.00
A6,Treatment,TargetGene1,1,1,1.98
B1,CK,RefGene1,2,1,1.95
B2,CK,RefGene2,2,1,2.00
B3,CK,TargetGene1,2,1,1.98
B4,Treatment,RefGene1,2,1,1.95
B5,Treatment,RefGene2,2,1,2.00
B6,Treatment,TargetGene1,2,1,1.98`;

    downloadFile(csv, 'design_with_efficiency_template.csv', 'text/csv');
    showNotification('含扩增效率的实验设计模板下载成功！文件保存在：Downloads/design_with_efficiency_template.csv', 'success');
}

/**
 * Download Design Template with Slope/Intercept (for Standard Curve method)
 */
function downloadDesignWithSlopeTemplate() {
    const csv = `Position,Treatment,Gene,Slope,Intercept,min.Cq,max.Cq
A1,CK,GAPDH,-0.97,31.95,20.53,30.84
A2,CK,Actin,-0.90,32.04,22.25,33.24
A3,CK,TargetGene1,-0.98,30.77,19.12,29.48
A4,Treatment,GAPDH,-0.97,31.95,20.53,30.84
A5,Treatment,Actin,-0.90,32.04,22.25,33.24
A6,Treatment,TargetGene1,-0.98,30.77,19.12,29.48
B1,CK,GAPDH,-0.97,31.95,20.53,30.84
B2,CK,Actin,-0.90,32.04,22.25,33.24
B3,CK,TargetGene1,-0.98,30.77,19.12,29.48
B4,Treatment,GAPDH,-0.97,31.95,20.53,30.84
B5,Treatment,Actin,-0.90,32.04,22.25,33.24
B6,Treatment,TargetGene1,-0.98,30.77,19.12,29.48`;

    downloadFile(csv, 'design_with_slope_intercept_template.csv', 'text/csv');
    showNotification('含斜率截距的实验设计模板下载成功！文件保存在：Downloads/design_with_slope_intercept_template.csv', 'success');
}

/**
 * Download Standard Curve Expression example data
 */
function downloadStandardCurveExpExample() {
    // Download Cq example
    const cqCsv = `Position	Cq
A1	27.83
A2	25.81
A3	34.97
A4	29.15
A5	31.96
A6	33.31
A7	28.97
A8	26.47
A9	31.89
A10	30.01
A11	34.94
A12	33.89
B1	27.57
B2	25.79
B3	32.04
B4	29.21
B5	33
B6	33.75
B7	29.02
B8	26.48
B9	30.91
B10	29.96
B11	33.76
B12	35.68
C1	27.53
C2	25.48
C3	32.29
C4	28.91
C5	34.54
C6	31.18
C7	27.02
C8	24.15
C9	29.19
C10	27.76
C11	29.6
C12	31.59
D1	27.3
D2	25.4
D3	31.22
D4	28.66
D5	32.07
D6	31.21
D7	27.25
D8	23.99
D9	29.37
D10	27.59
D11	30.53
D12	32`;

    downloadFile(cqCsv, 'cal_expre_rqpcr_cq.txt', 'text/csv');

    // Download Design example with efficiency
    setTimeout(() => {
        const designCsv = `Position	Group	Gene	BioRep	TechRep	Eff
A1	CK	OSPOX8	1	1	1.96
A2	CK	OsUBQ	1	1	1.77
A3	CK	OsWAK91	1	1	2.2
A4	CK	OsRBBI2	1	1	1.93
A5	CK	OsCeBip	1	1	1.83
A6	CK	OsPR10	1	1	1.87
A7	Treatment	OSPOX8	1	1	1.96
A8	Treatment	OsUBQ	1	1	1.77
A9	Treatment	OsWAK91	1	1	2.2
A10	Treatment	OsRBBI2	1	1	1.93
A11	Treatment	OsCeBip	1	1	1.83
A12	Treatment	OsPR10	1	1	1.87
B1	CK	OSPOX8	1	2	1.96
B2	CK	OsUBQ	1	2	1.77
B3	CK	OsWAK91	1	2	2.2
B4	CK	OsRBBI2	1	2	1.93
B5	CK	OsCeBip	1	2	1.83
B6	CK	OsPR10	1	2	1.87
B7	Treatment	OSPOX8	1	2	1.96
B8	Treatment	OsUBQ	1	2	1.77
B9	Treatment	OsWAK91	1	2	2.2
B10	Treatment	OsRBBI2	1	2	1.93
B11	Treatment	OsCeBip	1	2	1.83
B12	Treatment	OsPR10	1	2	1.87`;

        downloadFile(designCsv, 'cal_expre_rqpcr_design.txt', 'text/csv');
        showNotification('标曲表达量法示例数据下载完成！文件保存在：Downloads/cal_expre_rqpcr_cq.txt 和 cal_expre_rqpcr_design.txt', 'success');
    }, 500);
}


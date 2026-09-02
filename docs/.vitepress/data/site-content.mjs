const localize = (value, _locale) => value.zh ?? value

// Root-locale site: links are relative to the docs root, no locale prefix.
const withLocale = (_locale, link) => {
  if (/^https?:\/\//.test(link)) {
    return link
  }

  const normalized = link === '/' ? '/' : `/${link.replace(/^\/+/, '')}`
  return normalized
}

export const algorithmCatalog = [
  {
    id: 'huffman',
    slug: 'huffman',
    icon: '🌳',
    name: { zh: '霍夫曼编码' },
    chartLabel: { zh: 'Huffman' },
    description: {
      zh: '基于符号频率的最优前缀码。经典的无损压缩方法。',
    },
    compression: { zh: '中等' },
    speed: { zh: '快速' },
    compressionTag: { zh: '速度快' },
    speedLevel: 'fast',
    compressionLevel: 'medium',
    bestFor: {
      zh: ['文本文件', '通用数据', '自然语言'],
    },
  },
  {
    id: 'arithmetic',
    slug: 'arithmetic',
    icon: '🧮',
    name: { zh: '算术编码' },
    chartLabel: { zh: 'Arithmetic' },
    description: {
      zh: '整个消息编码为单个数字，压缩率接近理论熵限。',
    },
    compression: { zh: '高' },
    speed: { zh: '中速' },
    compressionTag: { zh: '高压缩率' },
    speedLevel: 'medium',
    compressionLevel: 'high',
    bestFor: {
      zh: ['最大压缩率', '统计型数据', '归档存储'],
    },
  },
  {
    id: 'range',
    slug: 'range',
    icon: '🎯',
    name: { zh: '区间编码' },
    chartLabel: { zh: 'Range' },
    description: {
      zh: '基于整数的算术编码，在速度与压缩率之间取得均衡。',
    },
    compression: { zh: '高' },
    speed: { zh: '快速' },
    compressionTag: { zh: '快 + 高压缩' },
    speedLevel: 'fast',
    compressionLevel: 'high',
    bestFor: {
      zh: ['生产系统', '实时压缩', '平衡型负载'],
    },
  },
  {
    id: 'rle',
    slug: 'rle',
    icon: '📏',
    name: { zh: '行程编码' },
    chartLabel: { zh: 'RLE' },
    description: {
      zh: '针对重复数据的简单快速压缩。常作为预处理步骤使用。',
    },
    compression: { zh: '可变' },
    speed: { zh: '极快' },
    compressionTag: { zh: '极快' },
    speedLevel: 'very-fast',
    compressionLevel: 'variable',
    bestFor: {
      zh: ['位图图像', '日志文件', '预处理步骤'],
    },
  },
  {
    id: 'lzss',
    slug: 'lzss',
    icon: '🔁',
    name: { zh: 'LZSS 字典编码' },
    chartLabel: { zh: 'LZSS' },
    description: {
      zh: '滑动窗口 + 距离引用的字典压缩，gzip 系算法的 LZ 基础。',
    },
    compression: { zh: '中等' },
    speed: { zh: '快速' },
    compressionTag: { zh: '结构感知' },
    speedLevel: 'fast',
    compressionLevel: 'medium',
    bestFor: {
      zh: ['重复片段数据', '文本与代码', '字典编码学习'],
    },
  },
]

export const benchmarkCatalog = {
  algorithms: algorithmCatalog.map(entry => ({
    id: entry.id,
    label: entry.chartLabel,
  })),
  languages: [
    { id: 'cpp', color: '#667eea', label: { zh: 'C++' } },
  ],
  datasets: [
    { id: 'textlike_10MiB', label: { zh: '类文本 (10 MiB)' } },
    { id: 'repetitive_10MiB', label: { zh: '重复数据 (10 MiB)' } },
    { id: 'random_1MiB', label: { zh: '随机 (1 MiB)' } },
  ],
  metrics: [
    { id: 'encodeSpeed', label: { zh: '编码速度 (MiB/s)' } },
    { id: 'decodeSpeed', label: { zh: '解码速度 (MiB/s)' } },
    { id: 'compressionRatio', label: { zh: '相对输入节省的体积' } },
  ],
  metricOptions: [
    { id: 'encodeSpeed', label: { zh: '编码速度' } },
    { id: 'decodeSpeed', label: { zh: '解码速度' } },
    { id: 'compressionRatio', label: { zh: '压缩比' } },
  ],
}

export const homepageFeatureCatalog = [
  ...algorithmCatalog.map(entry => ({
    id: entry.id,
    kind: 'algorithm',
    algorithmId: entry.id,
    title: entry.name,
    description: entry.description,
    tags: [
      { label: { zh: '了解更多' }, link: `/algorithms/${entry.slug}` },
      { label: entry.compressionTag },
    ],
  })),
  {
    id: 'benchmarks',
    kind: 'guide',
    title: { zh: '📊 性能基准' },
    description: {
      zh: '跨所有算法的性能基准测试。比较速度和压缩率。',
    },
    tags: [
      { label: { zh: '查看结果' }, link: '/benchmarks/results' },
      { label: { zh: '运行测试' }, link: '/benchmarks/how-to-run' },
    ],
  },
]

const navCatalog = [
  {
    id: 'home',
    text: { zh: '首页' },
    link: '/',
    activeMatch: { zh: '^/$' },
  },
  {
    id: 'guide',
    text: { zh: '开始' },
    link: '/guide/getting-started',
    activeMatch: { zh: '/guide/' },
  },
  {
    id: 'algorithms',
    text: { zh: '算法' },
    link: '/algorithms/huffman',
    activeMatch: { zh: '/algorithms/' },
  },
  {
    id: 'api',
    text: { zh: 'API' },
    link: '/api/cpp',
    activeMatch: { zh: '/api/' },
  },
  {
    id: 'benchmarks',
    text: { zh: '基准' },
    link: '/benchmarks/results',
    activeMatch: { zh: '/benchmarks/' },
  },
]

const sidebarCatalog = [
  {
    title: { zh: '开始使用' },
    items: [
      { text: { zh: '项目介绍' }, link: '/' },
      { text: { zh: '快速开始' }, link: '/guide/getting-started' },
    ],
  },
  {
    title: { zh: '算法说明' },
    items: [
      { text: { zh: '霍夫曼编码' }, link: '/algorithms/huffman' },
      { text: { zh: '算术编码' }, link: '/algorithms/arithmetic' },
      { text: { zh: '区间编码' }, link: '/algorithms/range' },
      { text: { zh: '行程编码' }, link: '/algorithms/rle' },
      { text: { zh: 'LZSS 字典编码' }, link: '/algorithms/lzss' },
    ],
  },
  {
    title: { zh: '架构' },
    items: [
      { text: { zh: '架构概览' }, link: '/architecture/' },
    ],
  },
  {
    title: { zh: 'API 参考' },
    items: [
      { text: { zh: 'C++ 头文件' }, link: '/api/cpp' },
    ],
  },
  {
    title: { zh: '基准测试' },
    items: [
      { text: { zh: '性能结果' }, link: '/benchmarks/results' },
      { text: { zh: '熵界对照' }, link: '/benchmarks/entropy' },
      { text: { zh: '经典 vs 现代' }, link: '/benchmarks/classic-vs-modern' },
      { text: { zh: '如何运行' }, link: '/benchmarks/how-to-run' },
    ],
  },
]

export function buildNav(locale) {
  return navCatalog.map(item => ({
    text: localize(item.text, locale),
    link: withLocale(locale, item.link),
    activeMatch: localize(item.activeMatch, locale),
  }))
}

export function buildSidebar(locale) {
  return sidebarCatalog.map(section => ({
    text: localize(section.title, locale),
    items: section.items.map(item => ({
      text: localize(item.text, locale),
      link: withLocale(locale, item.link),
    })),
  }))
}

export function getAlgorithmCards(locale) {
  return algorithmCatalog.map(entry => ({
    id: entry.id,
    slug: entry.slug,
    icon: entry.icon,
    name: localize(entry.name, locale),
    description: localize(entry.description, locale),
    compression: localize(entry.compression, locale),
    speed: localize(entry.speed, locale),
    bestFor: localize(entry.bestFor, locale),
    compressionLevel: entry.compressionLevel,
    speedLevel: entry.speedLevel,
    bestForLabel: localize({ zh: '适合场景：' }, locale),
    learnMoreLabel: localize({ zh: '了解更多' }, locale),
    compressionSuffix: localize({ zh: '压缩' }, locale),
    speedSuffix: localize({ zh: '速度' }, locale),
  }))
}

export function getBenchmarkContent(locale) {
  return {
    algorithms: benchmarkCatalog.algorithms.map(entry => ({
      id: entry.id,
      label: localize(entry.label, locale),
    })),
    languages: benchmarkCatalog.languages.map(entry => ({
      id: entry.id,
      label: localize(entry.label, locale),
      color: entry.color,
    })),
    datasets: benchmarkCatalog.datasets.map(entry => ({
      id: entry.id,
      label: localize(entry.label, locale),
    })),
    metrics: benchmarkCatalog.metrics.map(entry => ({
      id: entry.id,
      label: localize(entry.label, locale),
    })),
    metricOptions: benchmarkCatalog.metricOptions.map(entry => ({
      id: entry.id,
      label: localize(entry.label, locale),
    })),
    title: localize({ zh: '性能对比' }, locale),
    datasetLabel: localize({ zh: '数据集：' }, locale),
    metricLabel: localize({ zh: '指标：' }, locale),
    compressionNote: localize(
      {
        zh: '柱状图展示相对输入节省的体积，标签显示实际输出/输入比值，最佳压缩比会被高亮。',
      },
      locale
    ),
  }
}

export function getHomepageContent(locale) {
  return {
    subtitle: localize({ zh: '无损压缩算法库' }, locale),
    intro: localize(
      {
        zh: 'CompressKit 提供使用 C++17 实现的经典无损压缩算法。阅读实现、对每个算法运行相同输入、验证 round-trip 正确性。',
      },
      locale
    ),
    sections: {
      algorithms: localize({ zh: '算法' }, locale),
      quickStart: localize({ zh: '快速开始' }, locale),
    },
    quickStartCommand: 'git clone https://github.com/build-workbench/compress-kit.git && cd compress-kit && make build && make test',
    stats: ['C++17', 'CMake'],
    navLinks: [
      { text: localize({ zh: '快速开始' }, locale), link: withLocale(locale, '/guide/getting-started') },
      { text: 'GitHub', link: 'https://github.com/build-workbench/compress-kit' },
    ],
    featureCards: homepageFeatureCatalog.map(entry => ({
      id: entry.id,
      title: localize(entry.title, locale),
      description: localize(entry.description, locale),
      tags: entry.tags.map(tag => ({
        label: localize(tag.label, locale),
        link: tag.link ? withLocale(locale, tag.link) : null,
      })),
    })),
  }
}

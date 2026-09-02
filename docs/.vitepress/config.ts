import { defineConfig } from 'vitepress'
import { withMermaid } from 'vitepress-plugin-mermaid'
import llmstxt from 'vitepress-plugin-llms'
import footnote from 'markdown-it-footnote'
import mark from 'markdown-it-mark'
import { buildNav, buildSidebar } from './data/site-content.mjs'

const rawBase = process.env.VITEPRESS_BASE
const base = rawBase
  ? rawBase.startsWith('/')
    ? rawBase.endsWith('/') ? rawBase : `${rawBase}/`
    : `/${rawBase}/`
  : '/'

export default withMermaid(defineConfig({
  base,
  title: 'CompressKit',
  titleTemplate: ':title | CompressKit',
  description: '使用 C++17 实现的经典无损压缩算法。',
  cleanUrls: true,
  lastUpdated: true,
  appearance: true,

  sitemap: {
    hostname: 'https://build-workbench.github.io/compress-kit/',
  },

  locales: {
    root: {
      label: '简体中文',
      lang: 'zh-CN',
      themeConfig: {
        nav: buildNav('zh'),
        sidebar: buildSidebar('zh'),
        editLink: {
          pattern: 'https://github.com/build-workbench/compress-kit/edit/main/docs/:path',
          text: '在 GitHub 上编辑此页',
        },
        footer: false,
        outline: {
          level: [2, 3],
          label: '本页内容',
        },
        lastUpdated: {
          text: '最后更新',
        },
        docFooter: {
          prev: '上一页',
          next: '下一页',
        },
        returnToTopLabel: '返回顶部',
        sidebarMenuLabel: '菜单',
        darkModeSwitchLabel: '主题',
        search: {
          provider: 'local',
          options: {
            translations: {
              button: {
                buttonText: '搜索文档',
                buttonAriaLabel: '搜索文档',
              },
              modal: {
                noResultsText: '无法找到相关结果',
                resetButtonTitle: '清除查询条件',
                footer: {
                  selectText: '选择',
                  navigateText: '切换',
                  closeText: '关闭',
                },
              },
            },
          },
        },
      },
    },
  },

  themeConfig: {
    socialLinks: [
      { icon: 'github', link: 'https://github.com/build-workbench/compress-kit' },
    ],
    logo: {
      light: '/logo.svg',
      dark: '/logo-dark.svg',
      alt: 'CompressKit Logo'
    },
    siteTitle: 'CompressKit',
    externalLinkIcon: true,
  },

  markdown: {
    lineNumbers: true,
    languageAlias: {
      cuda: 'cpp',
    },
    config: (md) => {
      md.use(footnote)
      md.use(mark)
    }
  },

  head: [
    ['link', { rel: 'canonical', href: 'https://build-workbench.github.io/compress-kit/' }],
    ['meta', { charset: 'UTF-8' }],
    ['meta', { name: 'viewport', content: 'width=device-width, initial-scale=1.0' }],
    ['meta', { name: 'theme-color', content: '#2563eb', media: '(prefers-color-scheme: light)' }],
    ['meta', { name: 'theme-color', content: '#0f172a', media: '(prefers-color-scheme: dark)' }],
    ['meta', { name: 'keywords', content: 'compression algorithms, huffman coding, arithmetic coding, range coder, run-length encoding, C++17, lossless compression' }],
    ['meta', { name: 'author', content: 'CompressKit Team' }],
    ['meta', { name: 'robots', content: 'index, follow' }],
    ['meta', { property: 'og:type', content: 'website' }],
    ['meta', { property: 'og:locale', content: 'zh_CN' }],
    ['meta', { property: 'og:title', content: 'CompressKit | 经典无损压缩算法集' }],
    ['meta', { property: 'og:description', content: '使用 C++17 实现的经典无损压缩算法。' }],
    ['meta', { property: 'og:url', content: 'https://build-workbench.github.io/compress-kit/' }],
    ['meta', { property: 'og:site_name', content: 'CompressKit' }],
    ['meta', { property: 'og:image', content: '/compress-kit/og-image.svg' }],
    ['link', { rel: 'icon', type: 'image/svg+xml', href: '/compress-kit/logo.svg' }],
  ],

  vite: {
    plugins: [llmstxt()],
    resolve: {
      alias: {
        '@theme': '/.vitepress/theme',
      },
    },
  },
}))

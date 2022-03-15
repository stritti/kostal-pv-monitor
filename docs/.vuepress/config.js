const { path } = require('@vuepress/utils')

module.exports = {
  lang: 'en-US',
  title: 'Kostal PV Monitor',
  description: 'ESP32 based device that can be used to monitor the power usage of a Kostal Plenticore converter',
  repo: 'https://github.com/stritti/kostal-pv-monitor',
  theme: path.resolve(__dirname, './theme'),

  themeConfig: {
    sidebar: false,
    contributors: false,
    lastUpdated: false,
    navbar: [
      { text: 'Home', link: '/' },
      { text: 'GitHub', link: 'https://github.com/stritti/kostal-pv-monitor' },
    ]

  },
}

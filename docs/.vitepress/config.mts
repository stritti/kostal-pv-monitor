import { defineConfig } from 'vitepress'

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: "Kostal PV Monitor",
  description: "ESP32 based device that can be used to monitor the power usage of a Kostal Plenticore converter",
  base: '/kostal-pv-monitor/',
  
  themeConfig: {
    // https://vitepress.dev/reference/default-theme-config
    logo: '/img/kostal-pv-monitor.jpg',
    
    nav: [
      { text: 'Home', link: '/' },
      { text: 'GitHub', link: 'https://github.com/stritti/kostal-pv-monitor' }
    ],

    socialLinks: [
      { icon: 'github', link: 'https://github.com/stritti/kostal-pv-monitor' }
    ],
    
    footer: {
      message: 'MIT Licensed | Copyright 2022-present',
      copyright: 'Copyright © 2022-present <a href="https://twitter.com/_stritti_">Stephan Strittmatter</a>'
    }
  }
})

<template>
  <Layout>
    <template #page-bottom>
      <div class="theme-default-content">
        MIT Licensed |
        Copyright 2022-present |
        <a href="https://twitter.com/_stritti_">Stephan Strittmatter</a>
      </div>
    </template>
  </Layout>
</template>

<script>
import Layout from "@vuepress/theme-default/lib/client/layouts/Layout.vue";

export default {
  components: {
    Layout,
  },
};
</script>

<style lang="css">
.my-footer {
  text-align: center;
}
</style>
